/*
 * Copyright © 2013 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Red Hat
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.  Red
 * Hat makes no representations about the suitability of this software
 * for any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <stdlib.h>
#include <linux/input.h>
#include <libevdev/libevdev.h>
#include "touchpad-int.h"


static int
touchpad_update_abs_state(struct touchpad *tp,
			  const struct input_event *ev)
{
	int rc = 0;
	struct touch *t = touchpad_current_touch(tp);

	t->millis = timeval_to_millis(&ev->time);
	switch (ev->code) {
		case ABS_MT_POSITION_X:
			t->x = ev->value;
			t->dirty = true;
			tp->queued |= EVENT_MOTION;
			break;
		case ABS_MT_POSITION_Y:
			t->y = ev->value;
			t->dirty = true;
			tp->queued |= EVENT_MOTION;
			break;
		case ABS_MT_SLOT:
			tp->slot = ev->value;
			break;
		case ABS_MT_TRACKING_ID:
			if (ev->value == -1) {
				t->state = TOUCH_END;
				tp->fingers_down--;
			} else {
				struct touch *ptr = touchpad_pointer_touch(tp);
				if (ptr == NULL)
					t->pointer = true;

				t->state = TOUCH_BEGIN;
				t->number = tp->fingers_down++;
			}
			t->dirty = true;
			tp->queued |= EVENT_MOTION;
			break;
	}

	return rc;
}

static void
touchpad_update_button_state(struct touchpad *tp,
			     const struct input_event *ev)
{

	if (ev->code >= BTN_LEFT && ev->code <= BTN_TASK) {
		uint32_t mask;
		mask = 0x1 << (ev->code - BTN_LEFT);
		if (ev->value) {
			tp->buttons.state |= mask;
			tp->queued |= EVENT_BUTTON_PRESS;
		} else {
			tp->buttons.state &= ~mask;
			tp->queued |= EVENT_BUTTON_RELEASE;
		}
	}
}

static void
touchpad_unpin_finger(struct touchpad *tp)
{
	struct touch *t = touchpad_pinned_touch(tp);
	if (t) {
		t->pinned = false;
		if (tp->fingers_down == 1)
			t->pointer = true;
	}
}

static void
touchpad_pin_finger(struct touchpad *tp)
{
	struct touch *t;

	arg_require_flag_set(tp->queued, EVENT_BUTTON_PRESS);

	t = touchpad_pinned_touch(tp);
	if (t)
		return;

	if (tp->fingers_down == 1) /* Whoopee, the easy case */
		t = touchpad_pointer_touch(tp);
	else {
		int maxy = INT_MIN;
		struct touch *tmp, *new_pointer_touch = touchpad_touch(tp, 0);

		/* Pick the finger lowest to the bottom of the touchpad */
		touchpad_for_each_touch(tp, tmp) {
			printf("%d\n", tmp->y);
			if (tmp->y > maxy) {
				t = tmp;
				maxy = tmp->y;
			} else if (tmp->state == TOUCH_UPDATE ||
				   tmp->state == TOUCH_BEGIN)
				new_pointer_touch = tmp;
		}

		arg_require_not_null(new_pointer_touch);
		if (new_pointer_touch->state != TOUCH_NONE)
			new_pointer_touch->pointer = true;
	}

	arg_require_not_null(t);

	t->pinned = true;
	t->pointer = false;

	return ;
}

static void
touchpad_post_motion_events(struct touchpad *tp, void *userdata)
{
	struct touch *t;
	int dx, dy;

	if ((tp->queued & EVENT_MOTION) == 0)
		return;

	t = touchpad_pointer_touch(tp);
	if (t == NULL)
		return;

	touchpad_motion_dejitter(t);
	touchpad_motion_to_delta(t, &dx, &dy);

	if (dx || dy)
		tp->interface->motion(tp, userdata, dx, dy);

	touchpad_history_push(t, t->x, t->y, t->millis);

	if (t->state == TOUCH_END)
		touchpad_history_reset(t);
}

static void
touchpad_post_button_events(struct touchpad *tp, void *userdata)
{
	uint32_t current, old, shift;

	current = tp->buttons.state;
	old = tp->buttons.old_state;
	shift = 0;
	while (current || old) {
		if ((current & 0x1) ^ (old  & 0x1))
			tp->interface->button(tp, userdata, BTN_LEFT + shift, !!(current & 0x1));
		shift++;
		current >>= 1;
		old >>= 1;
	}
	tp->buttons.old_state = tp->buttons.state;
}

static void
touchpad_pre_process_touches(struct touchpad *tp)
{
	struct touch *t;

	touchpad_for_each_touch(tp, t)
		if (t->state == TOUCH_BEGIN)
			touchpad_history_push(t, t->x, t->y, t->millis);

	if (tp->queued & EVENT_BUTTON_PRESS)
		touchpad_pin_finger(tp);
}

static void
touchpad_post_process_touches(struct touchpad *tp)
{
	struct touch *t;
	int dec = 0;

	touchpad_for_each_touch(tp, t) {
		if (t->state == TOUCH_END) {
			t->state = TOUCH_NONE;
			t->pointer = false;
			t->pinned = false;
			dec++;
		} else if (t->state == TOUCH_BEGIN)
			t->state = TOUCH_UPDATE;

		t->dirty = false;
	}

	if (dec > 0) {
		touchpad_for_each_touch(tp, t)
			t->number -= dec;
	}

	if (tp->queued & EVENT_BUTTON_RELEASE)
		touchpad_unpin_finger(tp);

	tp->queued = EVENT_NONE;
}

static void
touchpad_post_events(struct touchpad *tp, void *userdata)
{
	touchpad_tap_handle_state(tp, userdata);
	if (touchpad_scroll_handle_state(tp, userdata) == 0) {
		touchpad_post_motion_events(tp, userdata);
		touchpad_post_button_events(tp, userdata);
	}
}

int
touchpad_handle_event(struct touchpad *tp,
		      void *userdata,
		      const struct input_event *ev)
{
	int rc = 0;

#if 0
	printf("%s %s: %d\n", libevdev_event_type_get_name(ev->type),
				libevdev_event_code_get_name(ev->type, ev->code),
				ev->value);
#endif

	switch(ev->type) {
		case EV_ABS:
			touchpad_update_abs_state(tp, ev);
			break;
		case EV_KEY:
			touchpad_update_button_state(tp, ev);
			break;
		/* we never get a SYN_DROPPED, it's filtered higer up */
		case EV_SYN:
			tp->ms = timeval_to_millis(&ev->time);
			touchpad_pre_process_touches(tp);
			touchpad_post_events(tp, userdata);
			touchpad_post_process_touches(tp);
			break;
	}

	return rc;
}


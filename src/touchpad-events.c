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


#define TOUCHPAD_FAKE_TRACKING_ID -1

static void
touchpad_begin_touch(struct touchpad *tp, struct touch *t, int tracking_id)
{
	static int fake_tracking_id = (1 << 16);

	/* A touch may be active from a fake touch, just overwrite the
	   values then. */
	if (t->state == TOUCH_NONE || t->state == TOUCH_END) {
		tp->fingers_down++;
		argcheck_int_ge(tp->fingers_down, 1);
	}


	if (t->state != TOUCH_UPDATE)
		t->state = TOUCH_BEGIN;

	if (tracking_id == TOUCHPAD_FAKE_TRACKING_ID) {
		tracking_id = fake_tracking_id++;
		t->fake = true;
	} else
		t->fake = false;

	t->number = tracking_id;
	t->dirty = true;
	tp->queued |= EVENT_MOTION;
}

static void
touchpad_end_touch(struct touchpad *tp, struct touch *t)
{
	if (t->state == TOUCH_NONE)
		return;

	t->state = TOUCH_END;
	tp->fingers_down--;
	argcheck_int_ge(tp->fingers_down, 0);
	t->dirty = true;
	tp->queued |= EVENT_MOTION;
}

static int
touchpad_update_abs_state(struct touchpad *tp,
			  const struct input_event *ev)
{
	int rc = 0;
	struct touch *t = touchpad_current_touch(tp);

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
			t = touchpad_current_touch(tp);
			break;
		case ABS_MT_TRACKING_ID:
			if (ev->value == -1)
				touchpad_end_touch(tp, t);
			else
				touchpad_begin_touch(tp, t, ev->value);
			break;
	}

	t->millis = timeval_to_millis(&ev->time);

	return rc;
}

static void
touchpad_begin_fake_touches(struct touchpad *tp, unsigned int code)
{
	int i;
	int tapcount = code - BTN_TOOL_DOUBLETAP + 2;
	struct touch *t;

	argcheck_int_range(code, BTN_TOOL_DOUBLETAP, BTN_TOOL_QUADTAP);

	/* Don't need to fake touches for this device */
	if (tp->maxtouches >= tapcount)
		return;

	for (i = 0; i < tapcount; i++) {
		t = touchpad_touch(tp, i);
		if (t->state == TOUCH_END) {
			touchpad_begin_touch(tp, t, TOUCHPAD_FAKE_TRACKING_ID);
			t->state = TOUCH_UPDATE;
		}
	}

	touchpad_begin_touch(tp, t, TOUCHPAD_FAKE_TRACKING_ID);
}

static void
touchpad_end_fake_touches(struct touchpad *tp, unsigned int code)
{
	int i;
	int tapcount = code - BTN_TOOL_DOUBLETAP + 2;
	struct touch *t;

	/* Don't need to fake touches for this device */
	if (tp->maxtouches >= tapcount)
		return;

	argcheck_int_range(code, BTN_TOOL_DOUBLETAP, BTN_TOOL_QUADTAP);

	/* FIXME: we should set a timer for this because if there
	   are still two fingers on the touchpad the touchpoints end in the
	   switch from TRIPLETAP to DOUBLETAP and are re-created in the next
	   event. A timer of even 2ms or so would help prevent this.
	 */
	for (i = 0; i < tapcount; i++) {
		t = touchpad_touch(tp, i);
		if (t->fake && (t->state == TOUCH_UPDATE || t->state == TOUCH_BEGIN))
			touchpad_end_touch(tp, t);
	}
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

	if (ev->code >= BTN_TOOL_DOUBLETAP && ev->code <= BTN_TOOL_QUADTAP) {
		if (ev->value)
			touchpad_begin_fake_touches(tp, ev->code);
		else
			touchpad_end_fake_touches(tp, ev->code);
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

	argcheck_flag_set(tp->queued, EVENT_BUTTON_PRESS);

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
			if (tmp->y > maxy) {
				t = tmp;
				maxy = tmp->y;
			} else if (tmp->state == TOUCH_UPDATE ||
				   tmp->state == TOUCH_BEGIN)
				new_pointer_touch = tmp;
		}

		argcheck_ptr_not_null(new_pointer_touch);
		if (new_pointer_touch->state != TOUCH_NONE)
			new_pointer_touch->pointer = true;
	}

	if (t) {
		t->pinned = true;
		t->pointer = false;
	}

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

	touchpad_motion_to_delta(t, &dx, &dy);

	if (dx || dy)
		tp->interface->motion(tp, userdata, dx, dy);

}

static void
touchpad_update_pointer_touch(struct touchpad *tp)
{
	struct touch *t;

	t = touchpad_pointer_touch(tp);
	if (t && t->state == TOUCH_END)
		t->pointer = false;
}

static void
touchpad_select_pointer_touch(struct touchpad *tp)
{
	struct touch *t = touchpad_pointer_touch(tp);

	if (t)
		return;

	touchpad_for_each_touch(tp, t) {
		if (touchpad_button_select_pointer_touch(tp, t)) {
			t->pointer = true;
			break;
		}
	}
}

static void
touchpad_pre_process_touches(struct touchpad *tp, void *userdata)
{
	struct touch *t;

	touchpad_select_pointer_touch(tp);

	touchpad_for_each_touch(tp, t) {
		if (t->state == TOUCH_BEGIN)
			touchpad_history_push(t, t->x, t->y, t->millis);
		if (t->state != TOUCH_NONE && t->dirty)
			touchpad_motion_dejitter(t);
	}

	if (tp->queued & EVENT_BUTTON_PRESS)
		touchpad_pin_finger(tp);
}

static void
touchpad_touch_reset(struct touchpad *tp, struct touch *t)
{
	t->state = TOUCH_NONE;
	t->pointer = false;
	t->pinned = false;
	t->fake = false;
	t->button_state = BUTTON_STATE_NONE;
	touchpad_history_reset(tp, t);
}

static void
touchpad_post_process_touches(struct touchpad *tp)
{
	struct touch *t;

	touchpad_for_each_touch(tp, t) {
		if (t->state == TOUCH_NONE)
			continue;

		touchpad_history_push(t, t->x, t->y, t->millis);

		if (t->state == TOUCH_END)
			touchpad_touch_reset(tp, t);
		 else if (t->state == TOUCH_BEGIN)
			t->state = TOUCH_UPDATE;

		t->dirty = false;
	}

	if (tp->queued & EVENT_BUTTON_RELEASE)
		touchpad_unpin_finger(tp);

	tp->queued = EVENT_NONE;

	touchpad_update_pointer_touch(tp);
}

static void
touchpad_post_events(struct touchpad *tp, void *userdata)
{
	touchpad_button_handle_state(tp, userdata);
	touchpad_tap_handle_state(tp, userdata);
	if (touchpad_scroll_handle_state(tp, userdata) == 0) {
		touchpad_post_motion_events(tp, userdata);
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
			if (tp->queued == EVENT_NONE)
				break;
			tp->ms = timeval_to_millis(&ev->time);
			touchpad_pre_process_touches(tp, userdata);
			touchpad_post_events(tp, userdata);
			touchpad_post_process_touches(tp);
			break;
	}

	return rc;
}


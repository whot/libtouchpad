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

	t->millis = ev->time.tv_usec * 1000;
	switch (ev->code) {
		case ABS_MT_POSITION_X:
			t->x = ev->value;
			t->dirty = true;
			break;
		case ABS_MT_POSITION_Y:
			t->y = ev->value;
			t->dirty = true;
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
		if (ev->value)
			tp->buttons.state |= mask;
		else
			tp->buttons.state &= ~mask;
	}
}

static void
touchpad_post_motion_events(struct touchpad *tp, void *userdata)
{
	struct touch *t = touchpad_pointer_touch(tp);
	int dx, dy;

	if (t == NULL || !t->dirty)
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
	int i;
	struct touch *t;

	for (i = 0, t = touchpad_touch(tp, i);
	     i < tp->ntouches; i++, t = touchpad_touch(tp, i)) {
		if (t->state == TOUCH_BEGIN)
			touchpad_history_push(t, t->x, t->y, t->millis);
	}
}

static void
touchpad_post_process_touches(struct touchpad *tp)
{
	int i;
	struct touch *t;

	for (i = 0, t = touchpad_touch(tp, i);
	     i < tp->ntouches; i++, t = touchpad_touch(tp, i)) {
		if (t->state == TOUCH_END)
			t->state = TOUCH_NONE;
		else if (t->state == TOUCH_BEGIN)
			t->state = TOUCH_UPDATE;

		t->dirty = false;
	}
}

static void
touchpad_post_events(struct touchpad *tp, void *userdata)
{
	int i;
	int dec = 0;
	struct touch *t;

	/* FIXME: broken
	touchpad_tap_handle_state(tp);
	*/

	touchpad_post_motion_events(tp, userdata);
	touchpad_post_button_events(tp, userdata);

	for (i = 0, t = touchpad_touch(tp, i);
	     i < tp->ntouches; i++, t = touchpad_touch(tp, i)) {
		if (t->state == TOUCH_END) {
			t->state = TOUCH_NONE;
			dec++;
		}
	}

	if (dec > 0) {
		for (i = 0, t = touchpad_touch(tp, i);
		     i < tp->ntouches; i++, t = touchpad_touch(tp, i))
			t->number -= dec;
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
			touchpad_pre_process_touches(tp);
			touchpad_post_events(tp, userdata);
			touchpad_post_process_touches(tp);
			break;
	}

	return rc;
}


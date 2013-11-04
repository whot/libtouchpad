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

#include <assert.h>
#include "touchpad-int.h"

static inline const char*
tap_state_to_str(enum tap_state state) {
	switch(state) {
		case TAP_STATE_IDLE: return "IDLE";
		case TAP_STATE_TOUCH: return "TOUCH";
		case TAP_STATE_TAPPED: return "TAPPED";
		case TAP_STATE_TOUCH_2: return "TOUCH_2";
		case TAP_STATE_TOUCH_3: return "TOUCH_3";
		case TAP_STATE_DRAGGING: return "DRAGGING";
		default:
			assert(state);
			break;
	}
	return NULL;
}


static void
touchpad_tap_idle_handle_event(struct touchpad *tp, enum tap_event event)
{
	switch (event) {
		case TAP_EVENT_TOUCH:
			tp->tap.state = TAP_STATE_TOUCH;
			break;
		default:
			assert(event);
			break;
	}
}

static void
touchpad_tap_touch_handle_event(struct touchpad *tp, enum tap_event event)
{
	switch (event) {
		case TAP_EVENT_TOUCH:
			tp->tap.state = TAP_STATE_TOUCH_2;
			break;
		case TAP_EVENT_RELEASE:
			tp->tap.state = TAP_STATE_TAPPED;
			printf("tap: button 1 down\n");
			break;
		case TAP_EVENT_TIMEOUT:
		case TAP_EVENT_MOTION:
			tp->tap.state = TAP_STATE_IDLE;
			break;
		default:
			assert(event);
			break;
	}
}

static void
touchpad_tap_tapped_handle_event(struct touchpad *tp, enum tap_event event)
{
	switch (event) {
		case TAP_EVENT_TOUCH:
			tp->tap.state = TAP_STATE_DRAGGING;
			break;
		case TAP_EVENT_TIMEOUT:
			tp->tap.state = TAP_STATE_IDLE;
			printf("tap: button 1 up\n");
			break;
		default:
			assert(event);
			break;
	}
}

static void
touchpad_tap_touch2_handle_event(struct touchpad *tp, enum tap_event event)
{
	switch (event) {
		case TAP_EVENT_TOUCH:
			tp->tap.state = TAP_STATE_TOUCH_3;
			break;
		case TAP_EVENT_RELEASE:
			tp->tap.state = TAP_STATE_IDLE;
			printf("tap: button 2 press\n");
			printf("tap: button 2 release\n");
			break;
		case TAP_EVENT_MOTION:
			tp->tap.state = TAP_STATE_DRAGGING;
			printf("tap: button 1 press\n");
			break;
		case TAP_EVENT_TIMEOUT:
			tp->tap.state = TAP_STATE_IDLE;
			break;
		default:
			assert(event);
	}
}

static void
touchpad_tap_touch3_handle_event(struct touchpad *tp, enum tap_event event)
{
	switch (event) {
		case TAP_EVENT_TOUCH:
		case TAP_EVENT_MOTION:
		case TAP_EVENT_TIMEOUT:
			tp->tap.state = TAP_STATE_IDLE;
			break;
		case TAP_EVENT_RELEASE:
			tp->tap.state = TAP_STATE_IDLE;
			printf("tap: button 3 press\n");
			printf("tap: button 3 release\n");
			break;
		default:
			assert(event);
	}
}

static void
touchpad_tap_dragging_handle_event(struct touchpad *tp, enum tap_event event)
{
	switch (event) {
		case TAP_EVENT_TOUCH:
			/* noop */
			break;
		case TAP_EVENT_RELEASE:
			tp->tap.state = TAP_STATE_IDLE;
			printf("tap: button 1 release\n");
			break;
		default:
			assert(1);
			break;
	}
}

static void
touchpad_tap_handle_event(struct touchpad *tp,
			  enum tap_event event)
{
	assert(event >= TAP_EVENT_NONE);

	printf("tap state: %s to ", tap_state_to_str(tp->tap.state));

	switch(tp->tap.state) {
		case TAP_STATE_IDLE:
			touchpad_tap_idle_handle_event(tp, event);
			break;
		case TAP_STATE_TOUCH:
			touchpad_tap_touch_handle_event(tp, event);
			break;
		case TAP_STATE_TAPPED:
			touchpad_tap_tapped_handle_event(tp, event);
			break;
		case TAP_STATE_TOUCH_2:
			touchpad_tap_touch2_handle_event(tp, event);
			break;
		case TAP_STATE_TOUCH_3:
			touchpad_tap_touch3_handle_event(tp, event);
			break;
		case TAP_STATE_DRAGGING:
			touchpad_tap_dragging_handle_event(tp, event);
			break;
		default:
			assert(1);
			break;
	}
	printf("%s\n", tap_state_to_str(tp->tap.state));
}

static bool
touchpad_tap_exceeds_motion_threshold(struct touchpad *tp, struct touch *t)
{
	int threshold = TAP_MOTION_TRESHOLD;
	int dx, dy;
	int rc;

	touchpad_motion_to_delta(t, &dx, &dy);

	rc = (dx * dx + dy * dy > threshold * threshold);
	if (rc) {
		printf("motion exceeded for %d/%d\n", dx, dy);
		printf("%d/%d %d/%d\n", t->x, t->y, touchpad_history_get(t, -1)->x, touchpad_history_get(t, -1)->y);
	}
	return rc;
}

int
touchpad_tap_handle_state(struct touchpad *tp)
{
	int i;
	for (i = 0; i < tp->ntouches; i++) {
		struct touch *t = touchpad_touch(tp, i);

		if (!t->dirty || t->state == TOUCH_NONE)
			continue;

		if (t->state == TOUCH_BEGIN)
			touchpad_tap_handle_event(tp, TAP_EVENT_TOUCH);
		else if (t->state == TOUCH_END)
			touchpad_tap_handle_event(tp, TAP_EVENT_RELEASE);
		else if (touchpad_tap_exceeds_motion_threshold(tp, t))
			touchpad_tap_handle_event(tp, TAP_EVENT_MOTION);
	}

	return 0;
}

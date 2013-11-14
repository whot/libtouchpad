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

#define CASE_RETURN_STRING(a) case a: return #a;

/*****************************************
 * DO NOT EDIT THIS FILE!
 *
 * Look at the state diagram in doc/touchpad-tap-state-machine.svg, or
 * online at
 * https://drive.google.com/file/d/0B1NwWmji69noYTdMcU1kTUZuUVE/edit?usp=sharing
 * (it's a http://draw.io diagram)
 *
 * Any changes in this file must be represented in the diagram.
 */

static inline const char*
tap_state_to_str(enum tap_state state) {
	switch(state) {
		CASE_RETURN_STRING(TAP_STATE_IDLE);
		CASE_RETURN_STRING(TAP_STATE_HOLD);
		CASE_RETURN_STRING(TAP_STATE_TOUCH);
		CASE_RETURN_STRING(TAP_STATE_TAPPED);
		CASE_RETURN_STRING(TAP_STATE_TOUCH_2);
		CASE_RETURN_STRING(TAP_STATE_TOUCH_2_HOLD);
		CASE_RETURN_STRING(TAP_STATE_TOUCH_3);
		CASE_RETURN_STRING(TAP_STATE_TOUCH_3_HOLD);
		CASE_RETURN_STRING(TAP_STATE_DRAGGING);
		CASE_RETURN_STRING(TAP_STATE_DRAGGING_OR_DOUBLETAP);
		CASE_RETURN_STRING(TAP_STATE_DRAGGING_2);
		CASE_RETURN_STRING(TAP_STATE_DEAD);
	}
	return NULL;
}

static inline const char*
tap_event_to_str(enum tap_event event) {
	switch(event) {
		CASE_RETURN_STRING(TAP_EVENT_NONE);
		CASE_RETURN_STRING(TAP_EVENT_TOUCH);
		CASE_RETURN_STRING(TAP_EVENT_MOTION);
		CASE_RETURN_STRING(TAP_EVENT_RELEASE);
		CASE_RETURN_STRING(TAP_EVENT_TIMEOUT);
	}
	return NULL;
}
#undef CASE_RETURN_STRING

static void
touchpad_tap_set_timer(struct touchpad *tp, void *userdata)
{
	tp->tap.timeout = tp->ms + tp->tap.config.timeout_period;
	tp->interface->register_timer(tp, userdata, tp->tap.config.timeout_period);
}

static void
touchpad_tap_clear_timer(struct touchpad *tp, void *userdata)
{
	tp->tap.timeout = 0;
	tp->interface->register_timer(tp, userdata, 0);
}

static void
touchpad_tap_idle_handle_event(struct touchpad *tp, enum tap_event event, void *userdata)
{
	switch (event) {
		case TAP_EVENT_TOUCH:
			tp->tap.state = TAP_STATE_TOUCH;
			tp->interface->register_timer(tp, userdata, tp->tap.config.timeout_period);
			tp->tap.timeout = tp->ms + tp->tap.config.timeout_period;
			break;
		case TAP_EVENT_RELEASE:
		case TAP_EVENT_TIMEOUT:
			break;
		default:
			log_bug(event, "invalid tap state %s\n", tap_event_to_str(event));
			break;
	}
}

static void
touchpad_tap_touch_handle_event(struct touchpad *tp, enum tap_event event, void *userdata)
{
	switch (event) {
		case TAP_EVENT_TOUCH:
			tp->tap.state = TAP_STATE_TOUCH_2;
			touchpad_tap_set_timer(tp, userdata);
			break;
		case TAP_EVENT_RELEASE:
			tp->tap.state = TAP_STATE_TAPPED;
			tp->interface->tap(tp, userdata, 1, true);
			touchpad_tap_set_timer(tp, userdata);
			break;
		case TAP_EVENT_TIMEOUT:
		case TAP_EVENT_MOTION:
			tp->tap.state = TAP_STATE_HOLD;
			touchpad_tap_clear_timer(tp, userdata);
			break;
		default:
			log_bug(event, "invalid tap state %s\n", tap_event_to_str(event));
			break;
	}
}

static void
touchpad_tap_hold_handle_event(struct touchpad *tp, enum tap_event event, void *userdata)
{
	switch (event) {
		case TAP_EVENT_TOUCH:
			tp->tap.state = TAP_STATE_TOUCH_2;
			touchpad_tap_set_timer(tp, userdata);
			break;
		case TAP_EVENT_RELEASE:
			tp->tap.state = TAP_STATE_IDLE;
			break;
		case TAP_EVENT_MOTION:
		case TAP_EVENT_TIMEOUT:
			break;
		default:
			log_bug(event, "invalid tap state %s\n", tap_event_to_str(event));
			break;
	}
}

static void
touchpad_tap_tapped_handle_event(struct touchpad *tp, enum tap_event event, void *userdata)
{
	switch (event) {
		case TAP_EVENT_TOUCH:
			tp->tap.state = TAP_STATE_DRAGGING_OR_DOUBLETAP;
			touchpad_tap_clear_timer(tp, userdata);
			break;
		case TAP_EVENT_TIMEOUT:
			tp->tap.state = TAP_STATE_IDLE;
			tp->interface->tap(tp, userdata, 1, false);
			break;
		default:
			log_bug(event, "invalid tap state %s\n", tap_event_to_str(event));
			break;
	}
}

static void
touchpad_tap_touch2_handle_event(struct touchpad *tp, enum tap_event event, void *userdata)
{
	switch (event) {
		case TAP_EVENT_TOUCH:
			tp->tap.state = TAP_STATE_TOUCH_3;
			touchpad_tap_set_timer(tp, userdata);
			break;
		case TAP_EVENT_RELEASE:
			tp->tap.state = TAP_STATE_HOLD;
			tp->interface->tap(tp, userdata, 2, true);
			tp->interface->tap(tp, userdata, 2, false);
			touchpad_tap_clear_timer(tp, userdata);
			break;
		case TAP_EVENT_MOTION:
		case TAP_EVENT_TIMEOUT:
			tp->tap.state = TAP_STATE_TOUCH_2_HOLD;
			break;
		default:
			log_bug(event, "invalid tap state %s\n", tap_event_to_str(event));
			break;
	}
}

static void
touchpad_tap_touch2_hold_handle_event(struct touchpad *tp, enum tap_event event, void *userdata)
{
	switch (event) {
		case TAP_EVENT_TOUCH:
			tp->tap.state = TAP_STATE_TOUCH_3;
			touchpad_tap_set_timer(tp, userdata);
			break;
		case TAP_EVENT_RELEASE:
			tp->tap.state = TAP_STATE_HOLD;
			break;
		case TAP_EVENT_MOTION:
		case TAP_EVENT_TIMEOUT:
			tp->tap.state = TAP_STATE_TOUCH_2_HOLD;
			break;
		default:
			log_bug(event, "invalid tap state %s\n", tap_event_to_str(event));
			break;
	}
}

static void
touchpad_tap_touch3_handle_event(struct touchpad *tp, enum tap_event event, void *userdata)
{
	switch (event) {
		case TAP_EVENT_TOUCH:
		case TAP_EVENT_MOTION:
		case TAP_EVENT_TIMEOUT:
			tp->tap.state = TAP_STATE_IDLE;
			touchpad_tap_clear_timer(tp, userdata);
			break;
		case TAP_EVENT_RELEASE:
			tp->tap.state = TAP_STATE_TOUCH_2_HOLD;
			tp->interface->tap(tp, userdata, 3, true);
			tp->interface->tap(tp, userdata, 3, false);
			break;
		default:
			log_bug(event, "invalid tap state %s\n", tap_event_to_str(event));
			break;
	}
}

static void
touchpad_tap_touch3_hold_handle_event(struct touchpad *tp, enum tap_event event, void *userdata)
{
	switch (event) {
		case TAP_EVENT_TOUCH:
			tp->tap.state = TAP_STATE_DEAD;
			touchpad_tap_set_timer(tp, userdata);
			break;
		case TAP_EVENT_RELEASE:
			tp->tap.state = TAP_STATE_TOUCH_2_HOLD;
			break;
		case TAP_EVENT_MOTION:
		case TAP_EVENT_TIMEOUT:
			break;
		default:
			log_bug(event, "invalid tap state %s\n", tap_event_to_str(event));
			break;
	}
}

static void
touchpad_tap_dragging_or_doubletap_handle_event(struct touchpad *tp, enum tap_event event, void *userdata)
{
	switch (event) {
		case TAP_EVENT_TOUCH:
			tp->tap.state = TAP_STATE_DRAGGING_2;
			break;
		case TAP_EVENT_RELEASE:
			tp->tap.state = TAP_STATE_IDLE;
			tp->interface->tap(tp, userdata, 1, false);
			tp->interface->tap(tp, userdata, 1, true);
			tp->interface->tap(tp, userdata, 1, false);
			touchpad_tap_clear_timer(tp, userdata);
			break;
		case TAP_EVENT_MOTION:
		case TAP_EVENT_TIMEOUT:
			tp->tap.state = TAP_STATE_DRAGGING;
			break;
		default:
			log_bug(event, "invalid tap state %s\n", tap_event_to_str(event));
			break;
	}
}

static void
touchpad_tap_dragging_handle_event(struct touchpad *tp, enum tap_event event, void *userdata)
{
	switch (event) {
		case TAP_EVENT_TOUCH:
			tp->tap.state = TAP_STATE_DRAGGING_2;
			break;
		case TAP_EVENT_RELEASE:
			tp->tap.state = TAP_STATE_IDLE;
			tp->interface->tap(tp, userdata, 1, false);
			break;
		case TAP_EVENT_MOTION:
		case TAP_EVENT_TIMEOUT:
			/* noop */
			break;
		default:
			log_bug(event, "invalid tap state %s\n", tap_event_to_str(event));
			break;
	}
}

static void
touchpad_tap_dragging2_handle_event(struct touchpad *tp, enum tap_event event, void *userdata)
{
	switch (event) {
		case TAP_EVENT_RELEASE:
			tp->tap.state = TAP_STATE_DRAGGING;
			break;
		case TAP_EVENT_TOUCH:
			tp->tap.state = TAP_STATE_DEAD;
			tp->interface->tap(tp, userdata, 1, false);
			break;
		case TAP_EVENT_MOTION:
		case TAP_EVENT_TIMEOUT:
			/* noop */
			break;
		default:
			log_bug(event, "invalid tap state %s\n", tap_event_to_str(event));
			break;
	}
}

static void
touchpad_tap_dead_handle_event(struct touchpad *tp, enum tap_event event, void *userdata)
{
	switch (event) {
		case TAP_EVENT_RELEASE:
			if (tp->fingers_down == 0)
				tp->tap.state = TAP_STATE_IDLE;
			break;
		case TAP_EVENT_TOUCH:
		case TAP_EVENT_MOTION:
		case TAP_EVENT_TIMEOUT:
			break;
		default:
			log_bug(event, "invalid tap state %s\n", tap_event_to_str(event));
			break;
	}
}

static void
touchpad_tap_handle_event(struct touchpad *tp, enum tap_event event, void *userdata)
{
	assert(event >= TAP_EVENT_NONE);

	if (!tp->tap.config.enabled)
		return;

	switch(tp->tap.state) {
		case TAP_STATE_IDLE:
			touchpad_tap_idle_handle_event(tp, event, userdata);
			break;
		case TAP_STATE_TOUCH:
			touchpad_tap_touch_handle_event(tp, event, userdata);
			break;
		case TAP_STATE_HOLD:
			touchpad_tap_hold_handle_event(tp, event, userdata);
			break;
		case TAP_STATE_TAPPED:
			touchpad_tap_tapped_handle_event(tp, event, userdata);
			break;
		case TAP_STATE_TOUCH_2:
			touchpad_tap_touch2_handle_event(tp, event, userdata);
			break;
		case TAP_STATE_TOUCH_2_HOLD:
			touchpad_tap_touch2_hold_handle_event(tp, event, userdata);
			break;
		case TAP_STATE_TOUCH_3:
			touchpad_tap_touch3_handle_event(tp, event, userdata);
			break;
		case TAP_STATE_TOUCH_3_HOLD:
			touchpad_tap_touch3_hold_handle_event(tp, event, userdata);
			break;
		case TAP_STATE_DRAGGING_OR_DOUBLETAP:
			touchpad_tap_dragging_or_doubletap_handle_event(tp, event, userdata);
			break;
		case TAP_STATE_DRAGGING:
			touchpad_tap_dragging_handle_event(tp, event, userdata);
			break;
		case TAP_STATE_DRAGGING_2:
			touchpad_tap_dragging2_handle_event(tp, event, userdata);
			break;
		case TAP_STATE_DEAD:
			touchpad_tap_dead_handle_event(tp, event, userdata);
			break;
		default:
			log_bug(event, "invalid tap state %s\n", tap_event_to_str(event));
			break;
	}
	if (tp->tap.state == TAP_STATE_IDLE)
		touchpad_tap_clear_timer(tp, userdata);

	log_debug("%s\n", tap_state_to_str(tp->tap.state));
}

static bool
touchpad_tap_exceeds_motion_threshold(struct touchpad *tp, struct touch *t)
{
	int threshold = tp->tap.config.move_threshold;
	int dx, dy;

	touchpad_motion_to_delta(t, &dx, &dy);

	return dx * dx + dy * dy > threshold * threshold;
}

int
touchpad_tap_handle_state(struct touchpad *tp, void *userdata)
{
	int i;
	for (i = 0; i < tp->ntouches; i++) {
		struct touch *t = touchpad_touch(tp, i);

		if (!t->dirty || t->state == TOUCH_NONE)
			continue;

		if (t->state == TOUCH_BEGIN)
			touchpad_tap_handle_event(tp, TAP_EVENT_TOUCH, userdata);
		else if (t->state == TOUCH_END)
			touchpad_tap_handle_event(tp, TAP_EVENT_RELEASE, userdata);
		else if (tp->tap.state != TAP_STATE_IDLE &&
			 touchpad_tap_exceeds_motion_threshold(tp, t))
			touchpad_tap_handle_event(tp, TAP_EVENT_MOTION, userdata);
	}

	return 0;
}

int
touchpad_tap_handle_timeout(struct touchpad *tp, unsigned int ms, void *userdata)
{
	if (!tp->tap.config.enabled)
		return 0;

	if (tp->tap.timeout && tp->tap.timeout <= ms)
		touchpad_tap_handle_event(tp, TAP_EVENT_TIMEOUT, userdata);

	return 0;
}

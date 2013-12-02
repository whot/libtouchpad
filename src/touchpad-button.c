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
#include <linux/input.h>
#include "touchpad-int.h"
#include "touchpad-util.h"

#define CASE_RETURN_STRING(a) case a: return #a;

static inline const char*
button_state_to_str(enum button_state state) {
	argcheck_int_range(state, BUTTON_STATE_NONE, BUTTON_STATE_PRESSED_LEFT);

	switch(state) {
		CASE_RETURN_STRING(BUTTON_STATE_NONE);
		CASE_RETURN_STRING(BUTTON_STATE_AREA);
		CASE_RETURN_STRING(BUTTON_STATE_LEFT);
		CASE_RETURN_STRING(BUTTON_STATE_LEFT_NEW);
		CASE_RETURN_STRING(BUTTON_STATE_RIGHT);
		CASE_RETURN_STRING(BUTTON_STATE_RIGHT_NEW);
		CASE_RETURN_STRING(BUTTON_STATE_LEFT_TO_AREA);
		CASE_RETURN_STRING(BUTTON_STATE_RIGHT_TO_AREA);
		CASE_RETURN_STRING(BUTTON_STATE_LEFT_TO_RIGHT);
		CASE_RETURN_STRING(BUTTON_STATE_RIGHT_TO_LEFT);
		CASE_RETURN_STRING(BUTTON_STATE_PRESSED_RIGHT);
		CASE_RETURN_STRING(BUTTON_STATE_PRESSED_LEFT);
	}
	return NULL;
}

static inline const char*
button_event_to_str(enum button_event event) {
	argcheck_int_range(event, BUTTON_EVENT_IN_R, BUTTON_EVENT_TIMEOUT);

	switch(event) {
		CASE_RETURN_STRING(BUTTON_EVENT_IN_R);
		CASE_RETURN_STRING(BUTTON_EVENT_IN_L);
		CASE_RETURN_STRING(BUTTON_EVENT_IN_AREA);
		CASE_RETURN_STRING(BUTTON_EVENT_UP);
		CASE_RETURN_STRING(BUTTON_EVENT_PRESS);
		CASE_RETURN_STRING(BUTTON_EVENT_RELEASE);
		CASE_RETURN_STRING(BUTTON_EVENT_TIMEOUT);
	}
	return NULL;
}

static inline bool
is_inside_area(struct touch *t, int top, int bottom, int left, int right)
{
	return (t->y >= top && t->y <= bottom &&
		t->x >= left && t->x <= right);
}

static inline bool
is_inside_button_area(struct touchpad *tp, struct touch *t)
{
	return t->y >= tp->buttons.config.top &&
	       t->y <= tp->buttons.config.bottom;
}

static inline bool
is_inside_right_area(struct touchpad *tp, struct touch *t)
{
	return is_inside_area(t, tp->buttons.config.top,
				 tp->buttons.config.bottom,
			         tp->buttons.config.right[0],
				 tp->buttons.config.right[1]);
}

static inline bool
is_inside_left_area(struct touchpad *tp, struct touch *t)
{
	return is_inside_button_area(tp, t) &&
	       !is_inside_right_area(tp, t);
}

static void
touchpad_button_set_enter_timer(struct touchpad *tp, struct touch *t, void *userdata)
{
	t->button_timeout = tp->ms + tp->buttons.config.enter_timeout;
	touchpad_request_timer(tp, userdata, tp->ms, tp->buttons.config.enter_timeout);
}

static void
touchpad_button_set_leave_timer(struct touchpad *tp, struct touch *t, void *userdata)
{
	t->button_timeout = tp->ms + tp->buttons.config.leave_timeout;
	touchpad_request_timer(tp, userdata, tp->ms, tp->buttons.config.leave_timeout);
}

static void
touchpad_button_clear_timer(struct touchpad *tp, struct touch *t, void *userdata)
{
	t->button_timeout = 0;
}

static void
touchpad_button_none_handle_event(struct touchpad *tp, struct touch *t,
				  enum button_event event, void *userdata)
{
	argcheck_int_range(event, BUTTON_EVENT_IN_R, BUTTON_EVENT_RELEASE);

	switch(event) {
		case BUTTON_EVENT_IN_R:
			t->button_state = BUTTON_STATE_RIGHT_NEW;
			touchpad_button_set_enter_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_IN_L:
			t->button_state = BUTTON_STATE_LEFT_NEW;
			touchpad_button_set_enter_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_IN_AREA:
			t->button_state = BUTTON_STATE_AREA;
			break;
		case BUTTON_EVENT_UP:
			break;
		case BUTTON_EVENT_PRESS:
		case BUTTON_EVENT_RELEASE:
		case BUTTON_EVENT_TIMEOUT:
			argcheck_not_reached();
			break;
	}
}
static void
touchpad_button_area_handle_event(struct touchpad *tp, struct touch *t,
				  enum button_event event, void *userdata)
{
	touchpad_button_clear_timer(tp, t, userdata);

	switch(event) {
		case BUTTON_EVENT_IN_R:
		case BUTTON_EVENT_IN_L:
		case BUTTON_EVENT_IN_AREA:
			break;
		case BUTTON_EVENT_UP:
			t->button_state = BUTTON_STATE_NONE;
			break;
		case BUTTON_EVENT_PRESS:
			t->button_state = BUTTON_STATE_PRESSED_LEFT;
			break;
		case BUTTON_EVENT_RELEASE:
		case BUTTON_EVENT_TIMEOUT:
			argcheck_not_reached();
			break;
	}
}
static void
touchpad_button_left_handle_event(struct touchpad *tp, struct touch *t,
				  enum button_event event, void *userdata)
{
	touchpad_button_clear_timer(tp, t, userdata);

	switch(event) {
		case BUTTON_EVENT_IN_R:
			t->button_state = BUTTON_STATE_LEFT_TO_RIGHT;
			touchpad_button_set_leave_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_IN_L:
			break;
		case BUTTON_EVENT_IN_AREA:
			t->button_state = BUTTON_STATE_LEFT_TO_AREA;
			touchpad_button_set_leave_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_UP:
			t->button_state = BUTTON_STATE_NONE;
			break;
		case BUTTON_EVENT_PRESS:
			t->button_state = BUTTON_STATE_PRESSED_LEFT;
			break;
		case BUTTON_EVENT_RELEASE:
		case BUTTON_EVENT_TIMEOUT:
			argcheck_not_reached();
			break;
	}
}
static void
touchpad_button_left_new_handle_event(struct touchpad *tp, struct touch *t,
				      enum button_event event, void *userdata)
{
	switch(event) {
		case BUTTON_EVENT_IN_R:
			t->button_state = BUTTON_STATE_RIGHT_NEW;
			touchpad_button_set_enter_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_IN_L:
			break;
		case BUTTON_EVENT_IN_AREA:
			t->button_state = BUTTON_STATE_LEFT_TO_AREA;
			touchpad_button_clear_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_UP:
			t->button_state = BUTTON_STATE_NONE;
			touchpad_button_clear_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_PRESS:
			t->button_state = BUTTON_STATE_PRESSED_LEFT;
			touchpad_button_clear_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_RELEASE:
			argcheck_not_reached();
			break;
		case BUTTON_EVENT_TIMEOUT:
			t->button_state = BUTTON_STATE_LEFT;
			break;
	}
}
static void
touchpad_button_right_handle_event(struct touchpad *tp, struct touch *t,
				   enum button_event event, void *userdata)
{
	touchpad_button_clear_timer(tp, t, userdata);

	switch(event) {
		case BUTTON_EVENT_IN_R:
			break;
		case BUTTON_EVENT_IN_L:
			t->button_state = BUTTON_STATE_RIGHT_TO_LEFT;
			touchpad_button_set_leave_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_IN_AREA:
			t->button_state = BUTTON_STATE_RIGHT_TO_AREA;
			touchpad_button_set_leave_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_UP:
			t->button_state = BUTTON_STATE_NONE;
			break;
		case BUTTON_EVENT_PRESS:
			t->button_state = BUTTON_STATE_PRESSED_RIGHT;
			break;
		case BUTTON_EVENT_TIMEOUT:
		case BUTTON_EVENT_RELEASE:
			argcheck_not_reached();
			break;
			break;
	}
}
static void
touchpad_button_right_new_handle_event(struct touchpad *tp, struct touch *t,
				       enum button_event event, void *userdata)
{
	switch(event) {
		case BUTTON_EVENT_IN_R:
			break;
		case BUTTON_EVENT_IN_L:
			t->button_state = BUTTON_STATE_LEFT_NEW;
			touchpad_button_set_enter_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_IN_AREA:
			t->button_state = BUTTON_STATE_AREA;
			touchpad_button_clear_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_UP:
			t->button_state = BUTTON_STATE_NONE;
			touchpad_button_clear_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_PRESS:
			t->button_state = BUTTON_STATE_PRESSED_RIGHT;
			touchpad_button_clear_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_RELEASE:
			argcheck_not_reached();
			break;
		case BUTTON_EVENT_TIMEOUT:
			t->button_state = BUTTON_STATE_RIGHT;
			break;
	}
}
static void
touchpad_button_left_to_area_handle_event(struct touchpad *tp, struct touch *t,
					  enum button_event event, void *userdata)
{
	switch(event) {
		case BUTTON_EVENT_IN_R:
			t->button_state = BUTTON_STATE_LEFT_TO_RIGHT;
			touchpad_button_set_leave_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_IN_L:
			t->button_state = BUTTON_STATE_LEFT;
			touchpad_button_clear_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_IN_AREA:
			break;
		case BUTTON_EVENT_UP:
			t->button_state = BUTTON_STATE_NONE;
			touchpad_button_clear_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_PRESS:
			t->button_state = BUTTON_STATE_PRESSED_LEFT;
			touchpad_button_clear_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_RELEASE:
			argcheck_not_reached();
			break;
		case BUTTON_EVENT_TIMEOUT:
			t->button_state = BUTTON_STATE_AREA;
			break;
	}
}
static void
touchpad_button_right_to_area_handle_event(struct touchpad *tp, struct touch *t,
					   enum button_event event, void *userdata)
{
	switch(event) {
		case BUTTON_EVENT_IN_R:
			t->button_state = BUTTON_STATE_RIGHT;
			touchpad_button_clear_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_IN_L:
			t->button_state = BUTTON_STATE_RIGHT_TO_LEFT;
			touchpad_button_set_leave_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_IN_AREA:
			break;
		case BUTTON_EVENT_UP:
			t->state = BUTTON_STATE_NONE;
			touchpad_button_clear_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_PRESS:
			t->button_state = BUTTON_STATE_PRESSED_RIGHT;
			touchpad_button_clear_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_RELEASE:
			argcheck_not_reached();
			break;
		case BUTTON_EVENT_TIMEOUT:
			t->button_state = BUTTON_STATE_AREA;
			break;
	}
}
static void
touchpad_button_left_to_right_handle_event(struct touchpad *tp, struct touch *t,
					   enum button_event event, void *userdata)
{
	switch(event) {
		case BUTTON_EVENT_IN_R:
			break;
		case BUTTON_EVENT_IN_L:
			t->button_state = BUTTON_STATE_LEFT;
			touchpad_button_clear_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_IN_AREA:
			t->button_state = BUTTON_STATE_LEFT_TO_AREA;
			touchpad_button_set_leave_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_UP:
			t->state = BUTTON_STATE_NONE;
			touchpad_button_clear_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_PRESS:
			t->button_state = BUTTON_STATE_PRESSED_LEFT;
			touchpad_button_clear_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_RELEASE:
			argcheck_not_reached();
			break;
		case BUTTON_EVENT_TIMEOUT:
			t->button_state = BUTTON_STATE_RIGHT;
			break;
	}
}
static void
touchpad_button_right_to_left_handle_event(struct touchpad *tp, struct touch *t,
					   enum button_event event, void *userdata)
{
	switch(event) {
		case BUTTON_EVENT_IN_R:
			t->state = BUTTON_STATE_RIGHT;
			touchpad_button_clear_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_IN_L:
			break;
		case BUTTON_EVENT_IN_AREA:
			t->state = BUTTON_STATE_RIGHT_TO_AREA;
			touchpad_button_set_leave_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_UP:
			t->state = BUTTON_STATE_NONE;
			touchpad_button_clear_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_PRESS:
			t->button_state = BUTTON_STATE_PRESSED_RIGHT;
			touchpad_button_clear_timer(tp, t, userdata);
			break;
		case BUTTON_EVENT_RELEASE:
			argcheck_not_reached();
			break;
		case BUTTON_EVENT_TIMEOUT:
			t->button_state = BUTTON_STATE_LEFT;
			break;
	}
}
static void
touchpad_button_pressed_right_handle_event(struct touchpad *tp, struct touch *t,
					   enum button_event event, void *userdata)
{
	touchpad_button_clear_timer(tp, t, userdata);
	switch(event) {
		case BUTTON_EVENT_IN_R:
		case BUTTON_EVENT_IN_L:
		case BUTTON_EVENT_IN_AREA:
		case BUTTON_EVENT_UP:
		case BUTTON_EVENT_PRESS:
			break;
		case BUTTON_EVENT_RELEASE:
			t->button_state = BUTTON_STATE_NONE;
			break;
		case BUTTON_EVENT_TIMEOUT:
			argcheck_not_reached();
			break;
	}
}
static void
touchpad_button_pressed_left_handle_event(struct touchpad *tp, struct touch *t,
					  enum button_event event, void *userdata)
{
	touchpad_button_clear_timer(tp, t, userdata);
	switch(event) {
		case BUTTON_EVENT_IN_R:
		case BUTTON_EVENT_IN_L:
		case BUTTON_EVENT_IN_AREA:
		case BUTTON_EVENT_UP:
		case BUTTON_EVENT_PRESS:
			break;
		case BUTTON_EVENT_RELEASE:
			t->button_state = BUTTON_STATE_NONE;
			break;
		case BUTTON_EVENT_TIMEOUT:
			argcheck_not_reached();
			break;
	}
}

static void
touchpad_button_handle_event(struct touchpad *tp, struct touch *t,
			     enum button_event event, void *userdata)
{
	argcheck_int_range(event, BUTTON_EVENT_IN_R, BUTTON_EVENT_TIMEOUT);


#if 0
	touchpad_log(tp, TOUCHPAD_LOG_DEBUG, "%d button state: from %s, event %s to:\n",
			t->number,
			button_state_to_str(t->button_state),
			button_event_to_str(event));
#endif

	switch(t->button_state) {
		case BUTTON_STATE_NONE:
			touchpad_button_none_handle_event(tp, t, event, userdata);
			break;
		case BUTTON_STATE_AREA:
			touchpad_button_area_handle_event(tp, t, event, userdata);
			break;
		case BUTTON_STATE_LEFT:
			touchpad_button_left_handle_event(tp, t, event, userdata);
			break;
		case BUTTON_STATE_LEFT_NEW:
			touchpad_button_left_new_handle_event(tp, t, event, userdata);
			break;
		case BUTTON_STATE_RIGHT:
			touchpad_button_right_handle_event(tp, t, event, userdata);
			break;
		case BUTTON_STATE_RIGHT_NEW:
			touchpad_button_right_new_handle_event(tp, t, event, userdata);
			break;
		case BUTTON_STATE_LEFT_TO_AREA:
			touchpad_button_left_to_area_handle_event(tp, t, event, userdata);
			break;
		case BUTTON_STATE_RIGHT_TO_AREA:
			touchpad_button_right_to_area_handle_event(tp, t, event, userdata);
			break;
		case BUTTON_STATE_LEFT_TO_RIGHT:
			touchpad_button_left_to_right_handle_event(tp, t, event, userdata);
			break;
		case BUTTON_STATE_RIGHT_TO_LEFT:
			touchpad_button_right_to_left_handle_event(tp, t, event, userdata);
			break;
		case BUTTON_STATE_PRESSED_RIGHT:
			touchpad_button_pressed_right_handle_event(tp, t, event, userdata);
			break;
		case BUTTON_STATE_PRESSED_LEFT:
			touchpad_button_pressed_left_handle_event(tp, t, event, userdata);
			break;
	}

#if 0
	touchpad_log(tp, TOUCHPAD_LOG_DEBUG, "	...%s\n", button_state_to_str(t->button_state));
#endif
}

int
touchpad_button_handle_state(struct touchpad *tp, void *userdata)
{
	struct touch *t;
	unsigned int button = BTN_LEFT;


	touchpad_for_each_touch(tp, t) {
		if (t->state == TOUCH_NONE)
			continue;

		if (t->state == TOUCH_END)
				touchpad_button_handle_event(tp, t, BUTTON_EVENT_UP, userdata);
		else if (t->dirty) {
			if (is_inside_right_area(tp, t))
				touchpad_button_handle_event(tp, t, BUTTON_EVENT_IN_R, userdata);
			else if (is_inside_left_area(tp, t))
				touchpad_button_handle_event(tp, t, BUTTON_EVENT_IN_L, userdata);
			else
				touchpad_button_handle_event(tp, t, BUTTON_EVENT_IN_AREA, userdata);
		}
		if (tp->queued & EVENT_BUTTON_RELEASE)
			touchpad_button_handle_event(tp, t, BUTTON_EVENT_RELEASE, userdata);
		if (tp->queued & EVENT_BUTTON_PRESS)
			touchpad_button_handle_event(tp, t, BUTTON_EVENT_PRESS, userdata);

		/* We don't post the button events during state machine
		   updates to support the finger-resting feature, i.e.
		   having a finger resting on the right-click area generates
		   a right-click. So we need to check if any of the touches
		   qualifies for a right click and remember that. Otherwise,
		   we post a left click.
		 */
		if (t->button_state == BUTTON_STATE_PRESSED_RIGHT)
			button = BTN_RIGHT;
	}

	if (tp->queued & EVENT_BUTTON_RELEASE)
		tp->interface->button(tp, userdata, tp->buttons.active_softbutton, false);

	if (tp->queued & EVENT_BUTTON_PRESS) {
		tp->interface->button(tp, userdata, button, true);
		tp->buttons.active_softbutton = button;
	}

	return 0;
}

int
touchpad_button_handle_timeout(struct touchpad *tp, unsigned int now, void *userdata)
{
	struct touch *t;
	int min_timeout = INT_MAX;


	touchpad_for_each_touch(tp, t) {
		if (t->button_timeout != 0 && t->button_timeout <= now) {
			touchpad_button_clear_timer(tp, t, userdata);
			touchpad_button_handle_event(tp, t, BUTTON_EVENT_TIMEOUT, userdata);
		}
		if (t->button_timeout != 0)
			min_timeout = min(t->button_timeout, min_timeout);
	}

	return min_timeout == INT_MAX ? 0 : min_timeout;
}

bool
touchpad_button_select_pointer_touch(struct touchpad *tp, struct touch *t)
{
	return t->button_state == BUTTON_STATE_AREA;
}

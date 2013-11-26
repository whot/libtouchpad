/*
 * Copyright © 2013 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include "tptest.h"
#include "touchpad-util.h"
#include "touchpad-config.h"

START_TEST(tap_single_finger)
{
	struct device *dev;
	union tptest_event *e;
	bool tap_down = false, tap_up = false;

	dev = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	tptest_touch_down(dev, 0, 3000, 3000);
	tptest_touch_up(dev, 0);


	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (e->button.is_press)
				tap_down = true;
			else
				tap_up = true;
			ck_assert_int_eq(e->button.button, 1);
		}
	}

	ck_assert(tap_down);
	ck_assert(tap_up);

	tptest_delete_device(dev);
}
END_TEST

START_TEST(tap_single_finger_move)
{
	struct device *dev;
	union tptest_event *e;
	bool tap_down = false, tap_up = false;

	dev = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	tptest_touch_down(dev, 0, 3000, 3000);
	tptest_touch_move_to(dev, 0, 3000, 3000, 4000, 3000, -1);
	tptest_touch_up(dev, 0);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (e->button.is_press)
				tap_down = true;
			else
				tap_up = true;
			ck_assert_int_eq(e->button.button, 1);
		}
	}

	ck_assert(!tap_down);
	ck_assert(!tap_up);

	tptest_delete_device(dev);
}
END_TEST

START_TEST(tap_single_finger_hold)
{
	struct device *dev;
	union tptest_event *e;
	bool tap_down = false, tap_up = false;
	int tap_timeout;

	dev = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	tptest_touch_down(dev, 0, 3000, 3000);

	while (tptest_handle_events(dev))
		;

	touchpad_config_get(dev->touchpad, TOUCHPAD_CONFIG_TAP_TIMEOUT,
			&tap_timeout, TOUCHPAD_CONFIG_NONE);

	usleep(tap_timeout * 2 * 1000);
	tptest_touch_up(dev, 0);

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (e->button.is_press)
				tap_down = true;
			else
				tap_up = true;
			ck_assert_int_eq(e->button.button, 1);
		}
	}

	ck_assert(!tap_down);
	ck_assert(!tap_up);

	tptest_delete_device(dev);
}
END_TEST

START_TEST(tap_single_finger_doubletap)
{
	struct device *dev;
	union tptest_event *e;
	int tap_down = 0, tap_up = 0;

	dev = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	tptest_touch_down(dev, 0, 3000, 3000);
	tptest_touch_up(dev, 0);
	tptest_touch_down(dev, 0, 3000, 3000);
	tptest_touch_up(dev, 0);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (e->button.is_press)
				tap_down++;
			else
				tap_up++;
			ck_assert_int_eq(e->button.button, 1);
		}
	}

	ck_assert(tap_down == 2);
	ck_assert(tap_up == 2);

	tptest_delete_device(dev);
}
END_TEST

START_TEST(tap_single_finger_tap_move)
{
	struct device *dev;
	union tptest_event *e;
	int tap_down = 0, tap_up = 0;

	dev = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	tptest_touch_down(dev, 0, 3000, 3000);
	tptest_touch_up(dev, 0);
	tptest_touch_down(dev, 0, 3000, 3000);
	tptest_touch_move_to(dev, 0, 3000, 3000, 4000, 3000, -1);
	tptest_touch_up(dev, 0);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (e->button.is_press)
				tap_down++;
			else
				tap_up++;
			ck_assert_int_eq(e->button.button, 1);
		}
	}

	ck_assert(tap_down == 1);
	ck_assert(tap_up == 1);

	tptest_delete_device(dev);
}
END_TEST

START_TEST(tap_single_finger_drag)
{
	struct device *dev;
	union tptest_event *e;
	int tap_down = 0, tap_up = 0;
	int button_state = 0;

	dev = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	tptest_touch_down(dev, 0, 3000, 3000);
	tptest_touch_up(dev, 0);
	tptest_touch_down(dev, 0, 3000, 3000);
	tptest_touch_move_to(dev, 0, 3000, 3000, 4000, 3000, -1);
	tptest_touch_up(dev, 0);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (e->button.is_press) {
				button_state |= (1 << (e->button.button - 1));
				tap_down++;
			} else {
				button_state &= ~(1 << (e->button.button -1));
				tap_up++;
			}
			ck_assert_int_eq(e->button.button, 1);
		}
		if (e->type == EVTYPE_MOTION)
			ck_assert_int_eq(button_state, 1);
	}

	ck_assert(tap_down == 1);
	ck_assert(tap_up == 1);

	tptest_delete_device(dev);
}
END_TEST

START_TEST(tap_single_finger_multi_drag)
{
	struct device *dev;
	union tptest_event *e;
	int tap_down = 0, tap_up = 0;
	int button_state = 0;

	dev = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	tptest_touch_down(dev, 0, 3000, 3000);
	tptest_touch_up(dev, 0);
	tptest_touch_down(dev, 0, 3000, 3000);
	tptest_touch_move_to(dev, 0, 3000, 3000, 4000, 3000, -1);
	tptest_touch_up(dev, 0);
	tptest_touch_down(dev, 0, 3000, 3000);
	tptest_touch_move_to(dev, 0, 3000, 3000, 4000, 3000, -1);
	tptest_touch_up(dev, 0);
	tptest_touch_down(dev, 0, 3000, 3000);
	tptest_touch_move_to(dev, 0, 3000, 3000, 4000, 3000, -1);
	tptest_touch_up(dev, 0);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (e->button.is_press) {
				button_state |= (1 << (e->button.button - 1));
				tap_down++;
			} else {
				button_state &= ~(1 << (e->button.button -1));
				tap_up++;
			}
			ck_assert_int_eq(e->button.button, 1);
		}
		if (e->type == EVTYPE_MOTION)
			ck_assert_int_eq(button_state, 1);
	}

	ck_assert(tap_down == 1);
	ck_assert(tap_up == 1);

	tptest_delete_device(dev);
}
END_TEST

START_TEST(tap_single_finger_read_delay)
{
	struct device *dev;
	union tptest_event *e;
	bool tap_down = false, tap_up = false;
	int tap_timeout;

	dev = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	tptest_touch_down(dev, 0, 3000, 3000);

	while (!tptest_handle_events(dev))
		;

	/* submit the events now, but don't read until later.
	   If the lib handles this correctly, the tap should
	   happen, even though we read after the timeout.
	 */
	tptest_touch_up(dev, 0);

	touchpad_config_get(dev->touchpad, TOUCHPAD_CONFIG_TAP_TIMEOUT,
			&tap_timeout, TOUCHPAD_CONFIG_NONE);

	usleep(tap_timeout * 2 * 1000);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (e->button.is_press)
				tap_down = true;
			else
				tap_up = true;
			ck_assert_int_eq(e->button.button, 1);
		}
	}

	ck_assert(tap_down);
	ck_assert(tap_up);

	tptest_delete_device(dev);
}
END_TEST

START_TEST(tap_double_finger)
{
	struct device *dev;
	bool tap_down = false, tap_up = false;
	union tptest_event *e;

	dev = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	tptest_touch_down(dev, 0, 3000, 3000);
	tptest_touch_down(dev, 1, 4000, 4000);
	tptest_touch_up(dev, 0);
	tptest_touch_up(dev, 1);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (e->button.is_press)
				tap_down = true;
			 else
				tap_up = true;
			 ck_assert_int_eq(e->button.button, 2);
		}
	}

	ck_assert(tap_down);
	ck_assert(tap_up);

	tptest_delete_device(dev);
}
END_TEST

START_TEST(tap_double_finger_invert_release)
{
	struct device *dev;
	bool tap_down = false, tap_up = false;
	union tptest_event *e;

	dev = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	tptest_touch_down(dev, 0, 3000, 3000);
	tptest_touch_down(dev, 1, 4000, 4000);
	/* same as tap_double_finger but touchpoints released in different
	   order */
	tptest_touch_up(dev, 1);
	tptest_touch_up(dev, 0);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (e->button.is_press)
				tap_down = true;
			 else
				tap_up = true;
			 ck_assert_int_eq(e->button.button, 2);
		}
	}

	ck_assert(tap_down);
	ck_assert(tap_up);

	tptest_delete_device(dev);
}
END_TEST

START_TEST(tap_double_finger_move)
{
	struct device *dev;
	bool tap_down = false, tap_up = false;
	union tptest_event *e;

	dev = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	tptest_touch_down(dev, 0, 3000, 3000);
	tptest_touch_down(dev, 1, 4000, 4000);
	tptest_touch_move_to(dev, 0, 3000, 3000, 4000, 3000, -1);
	tptest_touch_up(dev, 0);
	tptest_touch_up(dev, 1);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (e->button.is_press)
				tap_down = true;
			 else
				tap_up = true;
			 ck_assert_int_eq(e->button.button, 2);
		}
	}

	ck_assert(!tap_down);
	ck_assert(!tap_up);

	tptest_delete_device(dev);
}
END_TEST

START_TEST(tap_double_finger_hold)
{
	struct device *dev;
	bool tap_down = false, tap_up = false;
	union tptest_event *e;
	int tap_timeout;

	dev = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	tptest_touch_down(dev, 0, 3000, 3000);
	tptest_touch_down(dev, 1, 4000, 4000);

	while (tptest_handle_events(dev))
		;

	touchpad_config_get(dev->touchpad, TOUCHPAD_CONFIG_TAP_TIMEOUT,
			&tap_timeout, TOUCHPAD_CONFIG_NONE);

	usleep(tap_timeout * 2 * 1000);
	tptest_touch_up(dev, 0);
	tptest_touch_up(dev, 1);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (e->button.is_press)
				tap_down = true;
			 else
				tap_up = true;
			 ck_assert_int_eq(e->button.button, 2);
		}
	}

	ck_assert(!tap_down);
	ck_assert(!tap_up);

	tptest_delete_device(dev);
}
END_TEST

START_TEST(tap_double_finger_move_tap)
{
	struct device *dev;
	bool tap_down = false, tap_up = false;
	union tptest_event *e;

	dev = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	tptest_touch_down(dev, 0, 3000, 3000);
	tptest_touch_down(dev, 1, 4000, 4000);
	tptest_touch_up(dev, 1);
	tptest_touch_move_to(dev, 1, 4000, 4000, 4000, 3000, -1);
	tptest_touch_down(dev, 1, 4000, 4000);
	tptest_touch_up(dev, 1);

	while (tptest_handle_events(dev))
		;


	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (e->button.is_press)
				tap_down = true;
			 else
				tap_up = true;
			 ck_assert_int_eq(e->button.button, 2);
		}
	}

	ck_assert(tap_down);
	ck_assert(tap_up);

	tptest_delete_device(dev);
}
END_TEST

START_TEST(tap_double_finger_hold_tap)
{
	struct device *dev;
	bool tap_down = false, tap_up = false;
	union tptest_event *e;
	int tap_timeout;

	dev = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	tptest_touch_down(dev, 0, 3000, 3000);
	tptest_touch_down(dev, 1, 4000, 4000);

	while (tptest_handle_events(dev))
		;

	touchpad_config_get(dev->touchpad, TOUCHPAD_CONFIG_TAP_TIMEOUT,
			&tap_timeout, TOUCHPAD_CONFIG_NONE);

	usleep(tap_timeout * 2 * 1000);
	tptest_touch_up(dev, 1);
	tptest_touch_down(dev, 1, 4000, 4000);
	tptest_touch_up(dev, 1);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (e->button.is_press)
				tap_down = true;
			 else
				tap_up = true;
			 ck_assert_int_eq(e->button.button, 2);
		}
	}

	ck_assert(tap_down);
	ck_assert(tap_up);

	tptest_delete_device(dev);
}
END_TEST

int main(void) {
	tptest_add("tap", "tap_single_finger", tap_single_finger);
	tptest_add("tap", "tap_single_finger", tap_single_finger_move);
	tptest_add("tap", "tap_single_finger", tap_single_finger_hold);
	tptest_add("tap", "tap_single_finger", tap_single_finger_doubletap);
	tptest_add("tap", "tap_single_finger", tap_single_finger_tap_move);
	tptest_add("tap", "tap_single_finger", tap_single_finger_drag);
	tptest_add("tap", "tap_single_finger", tap_single_finger_multi_drag);
	tptest_add("tap", "tap_single_finger", tap_single_finger_read_delay);
	tptest_add("tap", "tap_double_finger", tap_double_finger);
	tptest_add("tap", "tap_double_finger", tap_double_finger_invert_release);
	tptest_add("tap", "tap_double_finger", tap_double_finger_move);
	tptest_add("tap", "tap_double_finger", tap_double_finger_hold);
	tptest_add("tap", "tap_double_finger", tap_double_finger_hold_tap);
	tptest_add("tap", "tap_double_finger", tap_double_finger_move_tap);
	return tptest_run();
}

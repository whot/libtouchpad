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
	struct tptest_device *dev;
	union tptest_event *e;
	bool tap_down = false, tap_up = false;
	int tap_timeout;

	dev = tptest_current_device();
	tptest_touch_down(dev, 0, 30, 30);
	tptest_touch_up(dev, 0);

	touchpad_config_get(dev->touchpad, TOUCHPAD_CONFIG_TAP_TIMEOUT,
			&tap_timeout, TOUCHPAD_CONFIG_NONE);
	usleep(tap_timeout * 2 * 1000);
	tptest_handle_events(dev);

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (tptest_tap_event(e)->is_press)
				tap_down = true;
			else
				tap_up = true;
			ck_assert_int_eq(tptest_tap_event(e)->fingers, 1);
		}
	}

	ck_assert(tap_down);
	ck_assert(tap_up);
}
END_TEST

START_TEST(tap_single_finger_move)
{
	struct tptest_device *dev;
	union tptest_event *e;
	bool tap_down = false, tap_up = false;

	dev = tptest_current_device();
	tptest_touch_down(dev, 0, 30, 30);
	tptest_touch_move_to(dev, 0, 30, 30, 40, 30, -1);
	tptest_touch_up(dev, 0);

	tptest_handle_events(dev);

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (tptest_tap_event(e)->is_press)
				tap_down = true;
			else
				tap_up = true;
			ck_assert_int_eq(tptest_tap_event(e)->fingers, 1);
		}
	}

	ck_assert(!tap_down);
	ck_assert(!tap_up);
}
END_TEST

START_TEST(tap_single_finger_hold)
{
	struct tptest_device *dev;
	union tptest_event *e;
	bool tap_down = false, tap_up = false;
	int tap_timeout;

	dev = tptest_current_device();
	tptest_touch_down(dev, 0, 30, 30);

	touchpad_config_get(dev->touchpad, TOUCHPAD_CONFIG_TAP_TIMEOUT,
			&tap_timeout, TOUCHPAD_CONFIG_NONE);

	usleep(tap_timeout * 2 * 1000);
	tptest_touch_up(dev, 0);

	tptest_handle_events(dev);

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (tptest_tap_event(e)->is_press)
				tap_down = true;
			else
				tap_up = true;
			ck_assert_int_eq(tptest_tap_event(e)->fingers, 1);
		}
	}

	ck_assert(!tap_down);
	ck_assert(!tap_up);
}
END_TEST

START_TEST(tap_single_finger_doubletap)
{
	struct tptest_device *dev;
	union tptest_event *e;
	int tap_down = 0, tap_up = 0;

	dev = tptest_current_device();
	tptest_touch_down(dev, 0, 30, 30);
	tptest_touch_up(dev, 0);
	tptest_touch_down(dev, 0, 30, 30);
	tptest_touch_up(dev, 0);

	tptest_handle_events(dev);

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (tptest_tap_event(e)->is_press)
				tap_down++;
			else
				tap_up++;
			ck_assert_int_eq(tptest_tap_event(e)->fingers, 1);
		}
	}

	ck_assert_int_eq(tap_down, 2);
	ck_assert_int_eq(tap_up, 2);
}
END_TEST

START_TEST(tap_single_finger_tap_move)
{
	struct tptest_device *dev;
	union tptest_event *e;
	int tap_down = 0, tap_up = 0;
	int tap_timeout;

	dev = tptest_current_device();
	tptest_touch_down(dev, 0, 30, 30);
	tptest_touch_up(dev, 0);
	tptest_touch_down(dev, 0, 30, 30);
	tptest_touch_move_to(dev, 0, 30, 30, 40, 30, -1);
	tptest_touch_up(dev, 0);

	touchpad_config_get(dev->touchpad, TOUCHPAD_CONFIG_TAP_TIMEOUT,
			&tap_timeout, TOUCHPAD_CONFIG_NONE);

	usleep(tap_timeout * 2 * 1000);
	tptest_handle_events(dev);

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (tptest_tap_event(e)->is_press)
				tap_down++;
			else
				tap_up++;
			ck_assert_int_eq(tptest_tap_event(e)->fingers, 1);
		}
	}

	ck_assert_int_eq(tap_down, 1);
	ck_assert_int_eq(tap_up, 1);
}
END_TEST

START_TEST(tap_single_finger_drag)
{
	struct tptest_device *dev;
	union tptest_event *e;
	int tap_down = 0, tap_up = 0;
	int finger_state = 0;
	int tap_timeout;

	dev = tptest_current_device();
	tptest_touch_down(dev, 0, 30, 30);
	tptest_touch_up(dev, 0);
	tptest_touch_down(dev, 0, 30, 30);
	tptest_touch_move_to(dev, 0, 30, 30, 50, 30, -1);
	tptest_touch_up(dev, 0);

	tptest_handle_events(dev);

	touchpad_config_get(dev->touchpad, TOUCHPAD_CONFIG_TAP_TIMEOUT,
			&tap_timeout, TOUCHPAD_CONFIG_NONE);

	usleep(tap_timeout * 2 * 1000);
	tptest_handle_events(dev);

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (tptest_tap_event(e)->is_press) {
				finger_state |= (1 << (tptest_tap_event(e)->fingers - 1));
				tap_down++;
			} else {
				finger_state &= ~(1 << (tptest_tap_event(e)->fingers -1));
				tap_up++;
			}
			ck_assert_int_eq(tptest_tap_event(e)->fingers, 1);
		}
		if (e->type == EVTYPE_MOTION)
			ck_assert_int_eq(finger_state, 1);
	}

	ck_assert_int_eq(tap_down, 1);
	ck_assert_int_eq(tap_up, 1);
}
END_TEST

START_TEST(tap_single_finger_multi_drag)
{
	struct tptest_device *dev;
	union tptest_event *e;
	int tap_down = 0, tap_up = 0;
	int finger_state = 0;
	int tap_timeout;

	dev = tptest_current_device();
	tptest_touch_down(dev, 0, 30, 30);
	tptest_touch_up(dev, 0);
	tptest_touch_down(dev, 0, 30, 30);
	tptest_touch_move_to(dev, 0, 30, 30, 40, 30, -1);
	tptest_touch_up(dev, 0);
	tptest_touch_down(dev, 0, 30, 30);
	tptest_touch_move_to(dev, 0, 30, 30, 40, 30, -1);
	tptest_touch_up(dev, 0);
	tptest_touch_down(dev, 0, 30, 30);
	tptest_touch_move_to(dev, 0, 30, 30, 40, 30, -1);
	tptest_touch_up(dev, 0);

	touchpad_config_get(dev->touchpad, TOUCHPAD_CONFIG_TAP_TIMEOUT,
			&tap_timeout, TOUCHPAD_CONFIG_NONE);

	usleep(tap_timeout * 2 * 1000);
	tptest_handle_events(dev);

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (tptest_tap_event(e)->is_press) {
				finger_state |= (1 << (tptest_tap_event(e)->fingers - 1));
				tap_down++;
			} else {
				finger_state &= ~(1 << (tptest_tap_event(e)->fingers -1));
				tap_up++;
			}
			ck_assert_int_eq(tptest_tap_event(e)->fingers, 1);
		}
		if (e->type == EVTYPE_MOTION)
			ck_assert_int_eq(finger_state, 1);
	}

	ck_assert_int_eq(tap_down, 1);
	ck_assert_int_eq(tap_up, 1);
}
END_TEST

START_TEST(tap_single_finger_read_delay)
{
	struct tptest_device *dev;
	union tptest_event *e;
	bool tap_down = false, tap_up = false;
	int tap_timeout;

	dev = tptest_current_device();
	tptest_touch_down(dev, 0, 30, 30);

	tptest_handle_events(dev);

	/* submit the events now, but don't read until later.
	   If the lib handles this correctly, the tap should
	   happen, even though we read after the timeout.
	 */
	tptest_touch_up(dev, 0);

	touchpad_config_get(dev->touchpad, TOUCHPAD_CONFIG_TAP_TIMEOUT,
			&tap_timeout, TOUCHPAD_CONFIG_NONE);

	usleep(tap_timeout * 2 * 1000);

	tptest_handle_events(dev);

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (tptest_tap_event(e)->is_press)
				tap_down = true;
			else
				tap_up = true;
			ck_assert_int_eq(tptest_tap_event(e)->fingers, 1);
		}
	}

	ck_assert(tap_down);
	ck_assert(tap_up);
}
END_TEST

START_TEST(tap_double_finger)
{
	struct tptest_device *dev;
	bool tap_down = false, tap_up = false;
	union tptest_event *e;

	dev = tptest_current_device();
	tptest_touch_down(dev, 0, 30, 30);
	tptest_touch_down(dev, 1, 40, 40);
	tptest_touch_up(dev, 0);
	tptest_touch_up(dev, 1);

	tptest_handle_events(dev);

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (tptest_tap_event(e)->is_press)
				tap_down = true;
			 else
				tap_up = true;
			 ck_assert_int_eq(tptest_tap_event(e)->fingers, 2);
		}
	}

	ck_assert(tap_down);
	ck_assert(tap_up);
}
END_TEST

START_TEST(tap_double_finger_invert_release)
{
	struct tptest_device *dev;
	bool tap_down = false, tap_up = false;
	union tptest_event *e;

	dev = tptest_current_device();
	tptest_touch_down(dev, 0, 30, 30);
	tptest_touch_down(dev, 1, 40, 40);
	/* same as tap_double_finger but touchpoints released in different
	   order */
	tptest_touch_up(dev, 1);
	tptest_touch_up(dev, 0);

	tptest_handle_events(dev);

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (tptest_tap_event(e)->is_press)
				tap_down = true;
			 else
				tap_up = true;
			 ck_assert_int_eq(tptest_tap_event(e)->fingers, 2);
		}
	}

	ck_assert(tap_down);
	ck_assert(tap_up);
}
END_TEST

START_TEST(tap_double_finger_move)
{
	struct tptest_device *dev;
	bool tap_down = false, tap_up = false;
	union tptest_event *e;

	dev = tptest_current_device();
	tptest_touch_down(dev, 0, 30, 30);
	tptest_touch_down(dev, 1, 40, 40);
	tptest_touch_move_to(dev, 0, 30, 30, 40, 30, -1);
	tptest_touch_up(dev, 0);
	tptest_touch_up(dev, 1);

	tptest_handle_events(dev);

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (tptest_tap_event(e)->is_press)
				tap_down = true;
			 else
				tap_up = true;
			 ck_assert_int_eq(tptest_tap_event(e)->fingers, 2);
		}
	}

	ck_assert(!tap_down);
	ck_assert(!tap_up);
}
END_TEST

START_TEST(tap_double_finger_hold)
{
	struct tptest_device *dev;
	bool tap_down = false, tap_up = false;
	union tptest_event *e;
	int tap_timeout;

	dev = tptest_current_device();
	tptest_touch_down(dev, 0, 30, 30);
	tptest_touch_down(dev, 1, 40, 40);

	tptest_handle_events(dev);

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
			if (tptest_tap_event(e)->is_press)
				tap_down = true;
			 else
				tap_up = true;
			 ck_assert_int_eq(tptest_tap_event(e)->fingers, 2);
		}
	}

	ck_assert(!tap_down);
	ck_assert(!tap_up);
}
END_TEST

START_TEST(tap_double_finger_move_tap)
{
	struct tptest_device *dev;
	bool tap_down = false, tap_up = false;
	union tptest_event *e;

	dev = tptest_current_device();
	tptest_touch_down(dev, 0, 30, 30);
	tptest_touch_down(dev, 1, 40, 40);
	tptest_touch_up(dev, 1);
	tptest_touch_move_to(dev, 1, 40, 40, 40, 30, -1);
	tptest_touch_down(dev, 1, 40, 40);
	tptest_touch_up(dev, 1);

	tptest_handle_events(dev);

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (tptest_tap_event(e)->is_press)
				tap_down = true;
			 else
				tap_up = true;
			 ck_assert_int_eq(tptest_tap_event(e)->fingers, 2);
		}
	}

	ck_assert(tap_down);
	ck_assert(tap_up);
}
END_TEST

START_TEST(tap_double_finger_hold_tap)
{
	struct tptest_device *dev;
	bool tap_down = false, tap_up = false;
	union tptest_event *e;
	int tap_timeout;

	dev = tptest_current_device();
	tptest_touch_down(dev, 0, 30, 30);
	tptest_touch_down(dev, 1, 40, 40);

	tptest_handle_events(dev);

	touchpad_config_get(dev->touchpad, TOUCHPAD_CONFIG_TAP_TIMEOUT,
			&tap_timeout, TOUCHPAD_CONFIG_NONE);

	usleep(tap_timeout * 2 * 1000);
	tptest_touch_up(dev, 1);
	tptest_touch_down(dev, 1, 40, 40);
	tptest_touch_up(dev, 1);

	tptest_handle_events(dev);

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (tptest_tap_event(e)->is_press)
				tap_down = true;
			 else
				tap_up = true;
			 ck_assert_int_eq(tptest_tap_event(e)->fingers, 2);
		}
	}

	ck_assert(tap_down);
	ck_assert(tap_up);
}
END_TEST

int main(int argc, char **argv) {
	tptest_add("tap_single_finger", tap_single_finger, TOUCHPAD_ALL_DEVICES);
	tptest_add("tap_single_finger", tap_single_finger_move, TOUCHPAD_ALL_DEVICES);
	tptest_add("tap_single_finger", tap_single_finger_hold, TOUCHPAD_ALL_DEVICES);
	tptest_add("tap_single_finger", tap_single_finger_doubletap, TOUCHPAD_ALL_DEVICES);
	tptest_add("tap_single_finger", tap_single_finger_tap_move, TOUCHPAD_ALL_DEVICES);
	tptest_add("tap_single_finger", tap_single_finger_drag, TOUCHPAD_ALL_DEVICES);
	tptest_add("tap_single_finger", tap_single_finger_multi_drag, TOUCHPAD_ALL_DEVICES);
	tptest_add("tap_single_finger", tap_single_finger_read_delay, TOUCHPAD_ALL_DEVICES);
	tptest_add("tap_double_finger", tap_double_finger, TOUCHPAD_ALL_MT_DEVICES);
	tptest_add("tap_double_finger", tap_double_finger_invert_release, TOUCHPAD_ALL_MT_DEVICES);
	tptest_add("tap_double_finger", tap_double_finger_move, TOUCHPAD_ALL_MT_DEVICES);
	tptest_add("tap_double_finger", tap_double_finger_hold, TOUCHPAD_ALL_MT_DEVICES);
	tptest_add("tap_double_finger", tap_double_finger_hold_tap, TOUCHPAD_ALL_MT_DEVICES);
	tptest_add("tap_double_finger", tap_double_finger_move_tap, TOUCHPAD_ALL_MT_DEVICES);
	return tptest_run(argc, argv);
}

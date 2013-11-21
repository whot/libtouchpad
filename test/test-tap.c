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

#include "test-common.h"
#include "touchpad-util.h"

START_TEST(tap_single_finger)
{
	struct device *dev;
	struct event *e;
	bool tap_down = false, tap_up = false;

	dev = test_common_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	test_common_touch_down(dev, 0, 3000, 3000);
	test_common_touch_up(dev, 0);


	while (test_common_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (e->is_press)
				tap_down = true;
			else
				tap_up = true;
			ck_assert_int_eq(e->button, 1);
		}
	}

	ck_assert(tap_down);
	ck_assert(tap_up);

	test_common_delete_device(dev);
}
END_TEST

START_TEST(tap_single_finger_move)
{
	struct device *dev;
	struct event *e;
	bool tap_down = false, tap_up = false;

	dev = test_common_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	test_common_touch_down(dev, 0, 3000, 3000);
	test_common_touch_move_to(dev, 0, 3000, 3000, 4000, 3000, -1);
	test_common_touch_up(dev, 0);

	while (test_common_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (e->is_press)
				tap_down = true;
			else
				tap_up = true;
			ck_assert_int_eq(e->button, 1);
		}
	}

	ck_assert(!tap_down);
	ck_assert(!tap_up);

	test_common_delete_device(dev);
}
END_TEST

START_TEST(tap_single_finger_doubletap)
{
	struct device *dev;
	struct event *e;
	int tap_down = 0, tap_up = 0;

	dev = test_common_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	test_common_touch_down(dev, 0, 3000, 3000);
	test_common_touch_up(dev, 0);
	test_common_touch_down(dev, 0, 3000, 3000);
	test_common_touch_up(dev, 0);

	while (test_common_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (e->is_press)
				tap_down++;
			else
				tap_up++;
			ck_assert_int_eq(e->button, 1);
		}
	}

	ck_assert(tap_down == 2);
	ck_assert(tap_up == 2);

	test_common_delete_device(dev);
}
END_TEST

START_TEST(tap_single_finger_tap_move)
{
	struct device *dev;
	struct event *e;
	int tap_down = 0, tap_up = 0;

	dev = test_common_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	test_common_touch_down(dev, 0, 3000, 3000);
	test_common_touch_up(dev, 0);
	test_common_touch_down(dev, 0, 3000, 3000);
	test_common_touch_move_to(dev, 0, 3000, 3000, 4000, 3000, -1);
	test_common_touch_up(dev, 0);

	while (test_common_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (e->is_press)
				tap_down++;
			else
				tap_up++;
			ck_assert_int_eq(e->button, 1);
		}
	}

	ck_assert(tap_down == 1);
	ck_assert(tap_up == 1);

	test_common_delete_device(dev);
}
END_TEST

START_TEST(tap_double_finger)
{
	struct device *dev;
	bool tap_down = false, tap_up = false;
	struct event *e;

	dev = test_common_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	test_common_touch_down(dev, 0, 3000, 3000);
	test_common_touch_down(dev, 1, 4000, 4000);
	test_common_touch_up(dev, 0);
	test_common_touch_up(dev, 1);

	while (test_common_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_TAP) {
			if (e->is_press)
				tap_down = true;
			 else
				tap_up = true;
			 ck_assert_int_eq(e->button, 2);
		}
	}

	ck_assert(tap_down);
	ck_assert(tap_up);

	test_common_delete_device(dev);
}
END_TEST

int main(void) {
	test_common_add("tap", "tap_single_finger", tap_single_finger);
	test_common_add("tap", "tap_single_finger", tap_single_finger_move);
	test_common_add("tap", "tap_single_finger", tap_single_finger_doubletap);
	test_common_add("tap", "tap_single_finger", tap_single_finger_tap_move);
	test_common_add("tap", "tap_double_finger", tap_double_finger);
	return test_common_run();
}

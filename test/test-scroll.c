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

#include "tptest.h"
#include "touchpad-util.h"
#include "touchpad-config.h"

START_TEST(scroll_two_finger_vert_down)
{
	struct tptest_device *dev;
	union tptest_event *e;
	union tptest_event *scroll = NULL;

	dev = tptest_current_device();
	tptest_touch_down(dev, 0, 2000, 2000);
	tptest_touch_down(dev, 1, 3000, 2000);
	tptest_touch_move_to(dev, 0, 2000, 2000, 2000, 4000, -1);
	tptest_touch_move_to(dev, 1, 3000, 2000, 3000, 4000, -1);
	tptest_touch_up(dev, 0);
	tptest_touch_up(dev, 1);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_SCROLL) {
			ck_assert_int_ge(tptest_scroll_event(e)->units, 0);
			ck_assert_int_eq(tptest_scroll_event(e)->dir, TOUCHPAD_SCROLL_VERTICAL);
			scroll = e;
		}
	}

	ck_assert(scroll != NULL);
	ck_assert(scroll->scroll.units == 0.0);
}
END_TEST

START_TEST(scroll_two_finger_vert_up)
{
	struct tptest_device *dev;
	union tptest_event *e;
	union tptest_event *scroll = NULL;

	dev = tptest_current_device();
	tptest_touch_down(dev, 0, 2000, 4000);
	tptest_touch_down(dev, 1, 3000, 4000);
	tptest_touch_move_to(dev, 0, 2000, 4000, 2000, 2000, -1);
	tptest_touch_move_to(dev, 1, 3000, 4000, 3000, 2000, -1);
	tptest_touch_up(dev, 0);
	tptest_touch_up(dev, 1);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_SCROLL) {
			ck_assert_int_le(tptest_scroll_event(e)->units, 0);
			ck_assert_int_eq(tptest_scroll_event(e)->dir, TOUCHPAD_SCROLL_VERTICAL);
			scroll = e;
		}
	}

	ck_assert(scroll != NULL);
	ck_assert(scroll->scroll.units == 0.0);
}
END_TEST

START_TEST(scroll_two_finger_single_finger_vert_down)
{
	struct tptest_device *dev;
	union tptest_event *e;
	union tptest_event *scroll = NULL;

	dev = tptest_current_device();
	tptest_touch_down(dev, 0, 2000, 2000);
	tptest_touch_down(dev, 1, 3000, 2000);
	tptest_touch_move_to(dev, 1, 3000, 2000, 3000, 4000, -1);
	tptest_touch_up(dev, 0);
	tptest_touch_up(dev, 1);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_SCROLL) {
			ck_assert_int_ge(tptest_scroll_event(e)->units, 0);
			ck_assert_int_eq(tptest_scroll_event(e)->dir, TOUCHPAD_SCROLL_VERTICAL);
			scroll = e;
		}
	}

	ck_assert(scroll != NULL);
	ck_assert(scroll->scroll.units == 0.0);
}
END_TEST

START_TEST(scroll_two_finger_single_finger_vert_up)
{
	struct tptest_device *dev;
	union tptest_event *e;
	union tptest_event *scroll = NULL;

	dev = tptest_current_device();
	tptest_touch_down(dev, 0, 2000, 4000);
	tptest_touch_down(dev, 1, 3000, 4000);
	tptest_touch_move_to(dev, 1, 3000, 4000, 3000, 2000, -1);
	tptest_touch_up(dev, 0);
	tptest_touch_up(dev, 1);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_SCROLL) {
			ck_assert_int_le(tptest_scroll_event(e)->units, 0);
			ck_assert_int_eq(tptest_scroll_event(e)->dir, TOUCHPAD_SCROLL_VERTICAL);
			scroll = e;
		}
	}

	ck_assert(scroll != NULL);
	ck_assert(scroll->scroll.units == 0.0);
}
END_TEST

START_TEST(scroll_two_finger_vert_lock)
{
	struct tptest_device *dev;
	union tptest_event *e;
	union tptest_event *scroll = NULL;

	dev = tptest_current_device();
	touchpad_config_set(dev->touchpad, NULL,
			    TOUCHPAD_CONFIG_SCROLL_METHOD,
			    TOUCHPAD_SCROLL_TWOFINGER_HORIZONTAL|TOUCHPAD_SCROLL_VERTICAL,
			    TOUCHPAD_CONFIG_NONE);

	tptest_touch_down(dev, 0, 2000, 2000);
	tptest_touch_down(dev, 1, 3000, 2000);
	tptest_touch_move_to(dev, 0, 2000, 2000, 2000, 3000, -1);
	tptest_touch_move_to(dev, 1, 3000, 2000, 3000, 3000, -1);
	/* horiz movement, must not cause events */
	tptest_touch_move_to(dev, 0, 2000, 3000, 4000, 3000, -1);
	tptest_touch_move_to(dev, 1, 3000, 3000, 4000, 3000, -1);
	tptest_touch_up(dev, 0);
	tptest_touch_up(dev, 1);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_SCROLL) {
			ck_assert_int_ge(tptest_scroll_event(e)->units, 0);
			ck_assert_int_eq(tptest_scroll_event(e)->dir, TOUCHPAD_SCROLL_VERTICAL);
			scroll = e;
		}
	}

	ck_assert(scroll != NULL);
	ck_assert(scroll->scroll.units == 0.0);
}
END_TEST

START_TEST(scroll_two_finger_horiz_right)
{
	struct tptest_device *dev;
	union tptest_event *e;
	union tptest_event *scroll = NULL;

	dev = tptest_current_device();

	touchpad_config_set(dev->touchpad, NULL,
			    TOUCHPAD_CONFIG_SCROLL_METHOD,
			    TOUCHPAD_SCROLL_TWOFINGER_HORIZONTAL,
			    TOUCHPAD_CONFIG_NONE);

	tptest_touch_down(dev, 0, 2000, 2000);
	tptest_touch_down(dev, 1, 2000, 3000);
	tptest_touch_move_to(dev, 0, 2000, 2000, 4000, 2000, -1);
	tptest_touch_move_to(dev, 1, 2000, 3000, 4000, 3000, -1);
	tptest_touch_up(dev, 0);
	tptest_touch_up(dev, 1);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_SCROLL) {
			ck_assert_int_ge(tptest_scroll_event(e)->units, 0);
			ck_assert_int_eq(tptest_scroll_event(e)->dir, TOUCHPAD_SCROLL_HORIZONTAL);
			scroll = e;
		}
	}

	ck_assert(scroll != NULL);
	ck_assert(scroll->scroll.units == 0.0);
}
END_TEST

START_TEST(scroll_two_finger_horiz_left)
{
	struct tptest_device *dev;
	union tptest_event *e;
	union tptest_event *scroll = NULL;

	dev = tptest_current_device();

	touchpad_config_set(dev->touchpad, NULL,
			    TOUCHPAD_CONFIG_SCROLL_METHOD,
			    TOUCHPAD_SCROLL_TWOFINGER_HORIZONTAL,
			    TOUCHPAD_CONFIG_NONE);

	tptest_touch_down(dev, 0, 4000, 2000);
	tptest_touch_down(dev, 1, 4000, 3000);
	tptest_touch_move_to(dev, 0, 4000, 2000, 2000, 2000, -1);
	tptest_touch_move_to(dev, 1, 4000, 3000, 2000, 3000, -1);
	tptest_touch_up(dev, 0);
	tptest_touch_up(dev, 1);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_SCROLL) {
			ck_assert_int_le(tptest_scroll_event(e)->units, 0);
			ck_assert_int_eq(tptest_scroll_event(e)->dir, TOUCHPAD_SCROLL_HORIZONTAL);
			scroll = e;
		}
	}

	ck_assert(scroll != NULL);
	ck_assert(scroll->scroll.units == 0.0);
}
END_TEST

START_TEST(scroll_two_finger_single_finger_horiz_right)
{
	struct tptest_device *dev;
	union tptest_event *e;
	union tptest_event *scroll = NULL;

	dev = tptest_current_device();

	touchpad_config_set(dev->touchpad, NULL,
			    TOUCHPAD_CONFIG_SCROLL_METHOD,
			    TOUCHPAD_SCROLL_TWOFINGER_HORIZONTAL,
			    TOUCHPAD_CONFIG_NONE);

	tptest_touch_down(dev, 0, 2000, 2000);
	tptest_touch_down(dev, 1, 2000, 3000);
	tptest_touch_move_to(dev, 1, 2000, 3000, 4000, 3000, -1);
	tptest_touch_up(dev, 0);
	tptest_touch_up(dev, 1);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_SCROLL) {
			ck_assert_int_ge(tptest_scroll_event(e)->units, 0);
			ck_assert_int_eq(tptest_scroll_event(e)->dir, TOUCHPAD_SCROLL_HORIZONTAL);
			scroll = e;
		}
	}

	ck_assert(scroll != NULL);
	ck_assert(scroll->scroll.units == 0.0);
}
END_TEST

START_TEST(scroll_two_finger_single_finger_horiz_left)
{
	struct tptest_device *dev;
	union tptest_event *e;
	union tptest_event *scroll = NULL;

	dev = tptest_current_device();

	touchpad_config_set(dev->touchpad, NULL,
			    TOUCHPAD_CONFIG_SCROLL_METHOD,
			    TOUCHPAD_SCROLL_TWOFINGER_HORIZONTAL,
			    TOUCHPAD_CONFIG_NONE);

	tptest_touch_down(dev, 0, 4000, 2000);
	tptest_touch_down(dev, 1, 4000, 3000);
	tptest_touch_move_to(dev, 1, 4000, 3000, 2000, 3000, -1);
	tptest_touch_up(dev, 0);
	tptest_touch_up(dev, 1);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_SCROLL) {
			ck_assert_int_le(tptest_scroll_event(e)->units, 0);
			ck_assert_int_eq(tptest_scroll_event(e)->dir, TOUCHPAD_SCROLL_HORIZONTAL);
			scroll = e;
		}
	}

	ck_assert(scroll != NULL);
	ck_assert(scroll->scroll.units == 0.0);
}
END_TEST

START_TEST(scroll_two_finger_horiz_lock)
{
	struct tptest_device *dev;
	union tptest_event *e;
	union tptest_event *scroll = NULL;

	dev = tptest_current_device();

	touchpad_config_set(dev->touchpad, NULL,
			    TOUCHPAD_CONFIG_SCROLL_METHOD,
			    TOUCHPAD_SCROLL_TWOFINGER_HORIZONTAL|TOUCHPAD_SCROLL_VERTICAL,
			    TOUCHPAD_CONFIG_NONE);

	tptest_touch_down(dev, 0, 2000, 2000);
	tptest_touch_down(dev, 1, 2000, 3000);
	tptest_touch_move_to(dev, 0, 2000, 2000, 4000, 2000, -1);
	tptest_touch_move_to(dev, 1, 2000, 3000, 4000, 3000, -1);
	/* vert movement, must not cause events */
	tptest_touch_move_to(dev, 0, 4000, 2000, 4000, 4000, -1);
	tptest_touch_move_to(dev, 1, 4000, 3000, 4000, 4000, -1);
	tptest_touch_up(dev, 0);
	tptest_touch_up(dev, 1);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_SCROLL) {
			ck_assert_int_ge(tptest_scroll_event(e)->units, 0);
			ck_assert_int_eq(tptest_scroll_event(e)->dir, TOUCHPAD_SCROLL_HORIZONTAL);
			scroll = e;
		}
	}

	ck_assert(scroll != NULL);
	ck_assert(scroll->scroll.units == 0.0);
}
END_TEST

int main(void) {
	tptest_add("scroll_two_finger_vert", scroll_two_finger_vert_down, TOUCHPAD_ALL_DEVICES);
	tptest_add("scroll_two_finger_vert", scroll_two_finger_vert_up, TOUCHPAD_ALL_DEVICES);
	tptest_add("scroll_two_finger_vert", scroll_two_finger_single_finger_vert_down, TOUCHPAD_ALL_DEVICES);
	tptest_add("scroll_two_finger_vert", scroll_two_finger_single_finger_vert_up, TOUCHPAD_ALL_DEVICES);
	tptest_add("scroll_two_finger_vert", scroll_two_finger_vert_lock, TOUCHPAD_ALL_DEVICES);

	tptest_add("scroll_two_finger_horiz", scroll_two_finger_horiz_left, TOUCHPAD_ALL_DEVICES);
	tptest_add("scroll_two_finger_horiz", scroll_two_finger_horiz_right, TOUCHPAD_ALL_DEVICES);
	tptest_add("scroll_two_finger_horiz", scroll_two_finger_single_finger_horiz_left, TOUCHPAD_ALL_DEVICES);
	tptest_add("scroll_two_finger_horiz", scroll_two_finger_single_finger_horiz_right, TOUCHPAD_ALL_DEVICES);
	tptest_add("scroll_two_finger_vert", scroll_two_finger_horiz_lock, TOUCHPAD_ALL_DEVICES);
	return tptest_run();
}

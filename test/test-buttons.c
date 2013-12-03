
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

#include <check.h>
#include <errno.h>
#include <assert.h>

#include "tptest.h"
#include "touchpad-util.h"
#include "touchpad-config.h"

START_TEST(left_click_generic)
{
	struct tptest_device *dev = tptest_current_device();
	union tptest_event *e;
	bool btn_up = false, btn_down = false;

	tptest_touch_down(dev, 0, 2000, 2000);
	tptest_click(dev, true);
	tptest_click(dev, false);
	tptest_touch_up(dev, 0);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_BUTTON) {
			ck_assert_int_ge(tptest_button_event(e)->button, BTN_LEFT);
			if (tptest_button_event(e)->is_press)
				btn_down = true;
			else
				btn_up = true;
		}
	}

	ck_assert(btn_down);
	ck_assert(btn_up);
}
END_TEST

START_TEST(left_click_in_area)
{
	struct tptest_device *dev = tptest_current_device();
	union tptest_event *e;
	bool btn_up = false, btn_down = false;

	ck_assert_int_eq(touchpad_config_set(dev->touchpad, NULL,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_LEFT, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_RIGHT, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_TOP, 40,
					     TOUCHPAD_CONFIG_SOFTBUTTON_BOTTOM, 100,
					     TOUCHPAD_CONFIG_NONE), 0);

	tptest_touch_down(dev, 0, 2000, 4000);
	tptest_click(dev, true);
	tptest_click(dev, false);
	tptest_touch_up(dev, 0);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_BUTTON) {
			ck_assert_int_ge(tptest_button_event(e)->button, BTN_LEFT);
			if (tptest_button_event(e)->is_press)
				btn_down = true;
			else
				btn_up = true;
		}
	}

	ck_assert(btn_down);
	ck_assert(btn_up);
}
END_TEST

START_TEST(left_click_in_area_with_rbtn)
{
	struct tptest_device *dev = tptest_current_device();
	union tptest_event *e;
	bool btn_up = false, btn_down = false;

	ck_assert_int_eq(touchpad_config_set(dev->touchpad, NULL,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_LEFT, 50,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_RIGHT, 100,
					     TOUCHPAD_CONFIG_SOFTBUTTON_TOP, 40,
					     TOUCHPAD_CONFIG_SOFTBUTTON_BOTTOM, 100,
					     TOUCHPAD_CONFIG_NONE), 0);

	tptest_touch_down(dev, 0, 2000, 4000);
	tptest_click(dev, true);
	tptest_click(dev, false);
	tptest_touch_up(dev, 0);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_BUTTON) {
			ck_assert_int_ge(tptest_button_event(e)->button, BTN_LEFT);
			if (tptest_button_event(e)->is_press)
				btn_down = true;
			else
				btn_up = true;
		}
	}

	ck_assert(btn_down);
	ck_assert(btn_up);
}
END_TEST

START_TEST(right_click_in_area)
{
	struct tptest_device *dev = tptest_current_device();
	union tptest_event *e;
	bool btn_up = false, btn_down = false;

	ck_assert_int_eq(touchpad_config_set(dev->touchpad, NULL,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_LEFT, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_RIGHT, 100,
					     TOUCHPAD_CONFIG_SOFTBUTTON_TOP, 40,
					     TOUCHPAD_CONFIG_SOFTBUTTON_BOTTOM, 100,
					     TOUCHPAD_CONFIG_NONE), 0);

	tptest_touch_down(dev, 0, 2000, 4000);
	tptest_click(dev, true);
	tptest_click(dev, false);
	tptest_touch_up(dev, 0);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_BUTTON) {
			ck_assert_int_ge(tptest_button_event(e)->button, BTN_RIGHT);
			if (tptest_button_event(e)->is_press)
				btn_down = true;
			else
				btn_up = true;
		}
	}

	ck_assert(btn_down);
	ck_assert(btn_up);
}
END_TEST

START_TEST(right_click_in_area_with_lbtn)
{
	struct tptest_device *dev = tptest_current_device();
	union tptest_event *e;
	bool btn_up = false, btn_down = false;

	ck_assert_int_eq(touchpad_config_set(dev->touchpad, NULL,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_LEFT, 50,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_RIGHT, 100,
					     TOUCHPAD_CONFIG_SOFTBUTTON_TOP, 40,
					     TOUCHPAD_CONFIG_SOFTBUTTON_BOTTOM, 100,
					     TOUCHPAD_CONFIG_NONE), 0);

	tptest_touch_down(dev, 0, 4000, 4000);
	tptest_click(dev, true);
	tptest_click(dev, false);
	tptest_touch_up(dev, 0);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_BUTTON) {
			ck_assert_int_ge(tptest_button_event(e)->button, BTN_RIGHT);
			if (tptest_button_event(e)->is_press)
				btn_down = true;
			else
				btn_up = true;
		}
	}

	ck_assert(btn_down);
	ck_assert(btn_up);
}
END_TEST

START_TEST(right_click_whole_touchpad)
{
	struct tptest_device *dev = tptest_current_device();
	union tptest_event *e;
	bool btn_up = false, btn_down = false;

	ck_assert_int_eq(touchpad_config_set(dev->touchpad, NULL,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_LEFT, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_RIGHT, 100,
					     TOUCHPAD_CONFIG_SOFTBUTTON_TOP, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_BOTTOM, 100,
					     TOUCHPAD_CONFIG_NONE), 0);

	tptest_touch_down(dev, 0, 2000, 2000);
	tptest_click(dev, true);
	tptest_click(dev, false);
	tptest_touch_up(dev, 0);

	while (tptest_handle_events(dev))
		;

	ARRAY_FOR_EACH(dev->events, e) {
		if (e->type == EVTYPE_NONE)
			break;
		if (e->type == EVTYPE_BUTTON) {
			ck_assert_int_ge(tptest_button_event(e)->button, BTN_RIGHT);
			if (tptest_button_event(e)->is_press)
				btn_down = true;
			else
				btn_up = true;
		}
	}

	ck_assert(btn_down);
	ck_assert(btn_up);
}
END_TEST

int main(void) {
	tptest_add("buttons_left_click", left_click_generic, TOUCHPAD_ALL_DEVICES);
	tptest_add("buttons_left_click", left_click_in_area, TOUCHPAD_ALL_DEVICES);
	tptest_add("buttons_left_click", left_click_in_area_with_rbtn, TOUCHPAD_ALL_DEVICES);

	tptest_add("buttons_right_click", right_click_in_area, TOUCHPAD_ALL_DEVICES);
	tptest_add("buttons_right_click", right_click_in_area_with_lbtn, TOUCHPAD_ALL_DEVICES);
	tptest_add("buttons_right_click", right_click_whole_touchpad, TOUCHPAD_ALL_DEVICES);

	return tptest_run();
}

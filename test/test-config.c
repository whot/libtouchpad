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
#include "touchpad.h"
#include "touchpad-util.h"
#include "touchpad-config.h"

START_TEST(config_get)
{
	int value;

	struct tptest_device *dev = tptest_current_device();

	for (int i = TOUCHPAD_CONFIG_NONE + 1; i < TOUCHPAD_CONFIG_LAST; i++)
		ck_assert_int_eq(touchpad_config_get(dev->touchpad,
						     i, &value,
						     TOUCHPAD_CONFIG_NONE), 0);
}
END_TEST

START_TEST(config_get_invalid)
{
	int value;
	struct tptest_device *dev = tptest_current_device();

	tptest_allow_errors(true);
	ck_assert_int_eq(touchpad_config_get(dev->touchpad,
					     TOUCHPAD_CONFIG_LAST, &value,
					     TOUCHPAD_CONFIG_NONE), 1);
	ck_assert_int_eq(touchpad_config_get(dev->touchpad,
					     TOUCHPAD_CONFIG_USE_DEFAULT, &value,
					     TOUCHPAD_CONFIG_NONE), 1);
	ck_assert_int_eq(touchpad_config_get(dev->touchpad,
					     TOUCHPAD_CONFIG_NONE), 0);
	tptest_allow_errors(false);
}
END_TEST

START_TEST(config_get_empty)
{
	int value = 10;
	struct tptest_device *dev = tptest_current_device();

	ck_assert_int_eq(touchpad_config_get(dev->touchpad, TOUCHPAD_CONFIG_NONE, &value), 0);
	ck_assert_int_eq(value, 10);
}
END_TEST

START_TEST(config_set_tap_enabled)
{
	enum touchpad_config_error error;
	struct tptest_device *dev = tptest_current_device();
	int value;
	enum touchpad_config_parameter p = TOUCHPAD_CONFIG_TAP_ENABLE;

	ck_assert_int_eq(touchpad_config_set(dev->touchpad, &error, p, 0,
					     TOUCHPAD_CONFIG_NONE), 0);
	ck_assert_int_eq(error, TOUCHPAD_CONFIG_ERROR_NO_ERROR);
	ck_assert_int_eq(touchpad_config_get(dev->touchpad, p, &value, TOUCHPAD_CONFIG_NONE), 0);
	ck_assert_int_eq(value, 0);

	ck_assert_int_eq(touchpad_config_set(dev->touchpad, &error, p, 1,
					     TOUCHPAD_CONFIG_NONE), 0);
	ck_assert_int_eq(error, TOUCHPAD_CONFIG_ERROR_NO_ERROR);
	ck_assert_int_eq(touchpad_config_get(dev->touchpad, p, &value, TOUCHPAD_CONFIG_NONE), 0);
	ck_assert_int_eq(value, 1);

	ck_assert_int_eq(touchpad_config_set(dev->touchpad, &error, p, TOUCHPAD_CONFIG_USE_DEFAULT,
					     TOUCHPAD_CONFIG_NONE), 0);
	ck_assert_int_eq(error, TOUCHPAD_CONFIG_ERROR_NO_ERROR);
	ck_assert_int_eq(touchpad_config_get(dev->touchpad, p, &value, TOUCHPAD_CONFIG_NONE), 0);
	ck_assert_int_eq(value, 1);
}
END_TEST

START_TEST(config_buttons_get_defaults)
{
	struct tptest_device *dev = tptest_current_device();
	int values[4];

	ck_assert_int_eq(touchpad_config_get(dev->touchpad,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_LEFT, &values[0],
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_RIGHT, &values[1],
					     TOUCHPAD_CONFIG_SOFTBUTTON_TOP, &values[2],
					     TOUCHPAD_CONFIG_SOFTBUTTON_BOTTOM, &values[3],
					     TOUCHPAD_CONFIG_NONE), 0);
	ck_assert_int_ge(values[0], 0);
	ck_assert_int_ge(values[1], 0);
	ck_assert_int_ge(values[2], 0);
	ck_assert_int_ge(values[3], 0);
	ck_assert_int_le(values[0], 100);
	ck_assert_int_le(values[1], 100);
	ck_assert_int_le(values[2], 100);
	ck_assert_int_le(values[3], 100);
	ck_assert_int_le(values[0], values[1]);
	ck_assert_int_le(values[2], values[3]);
}
END_TEST

START_TEST(config_buttons_set_invalid)
{
	struct tptest_device *dev = tptest_current_device();
	enum touchpad_config_error error;

	ck_assert_int_eq(touchpad_config_set(dev->touchpad, &error,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_LEFT, 101,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_RIGHT, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_TOP, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_BOTTOM, 0,
					     TOUCHPAD_CONFIG_NONE), 1);
	ck_assert_int_eq(error, TOUCHPAD_CONFIG_ERROR_VALUE_TOO_HIGH);

	ck_assert_int_eq(touchpad_config_set(dev->touchpad, &error,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_LEFT, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_RIGHT, 101,
					     TOUCHPAD_CONFIG_SOFTBUTTON_TOP, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_BOTTOM, 0,
					     TOUCHPAD_CONFIG_NONE), 2);
	ck_assert_int_eq(error, TOUCHPAD_CONFIG_ERROR_VALUE_TOO_HIGH);

	ck_assert_int_eq(touchpad_config_set(dev->touchpad, &error,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_LEFT, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_RIGHT, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_TOP, 101,
					     TOUCHPAD_CONFIG_SOFTBUTTON_BOTTOM, 0,
					     TOUCHPAD_CONFIG_NONE), 3);
	ck_assert_int_eq(error, TOUCHPAD_CONFIG_ERROR_VALUE_TOO_HIGH);

	ck_assert_int_eq(touchpad_config_set(dev->touchpad, &error,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_LEFT, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_RIGHT, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_TOP, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_BOTTOM, 101,
					     TOUCHPAD_CONFIG_NONE), 4);
	ck_assert_int_eq(error, TOUCHPAD_CONFIG_ERROR_VALUE_TOO_HIGH);

	ck_assert_int_eq(touchpad_config_set(dev->touchpad, &error,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_LEFT, -1,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_RIGHT, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_TOP, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_BOTTOM, 0,
					     TOUCHPAD_CONFIG_NONE), 1);
	ck_assert_int_eq(error, TOUCHPAD_CONFIG_ERROR_VALUE_TOO_LOW);

	ck_assert_int_eq(touchpad_config_set(dev->touchpad, &error,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_LEFT, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_RIGHT, -1,
					     TOUCHPAD_CONFIG_SOFTBUTTON_TOP, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_BOTTOM, 0,
					     TOUCHPAD_CONFIG_NONE), 2);
	ck_assert_int_eq(error, TOUCHPAD_CONFIG_ERROR_VALUE_TOO_LOW);

	ck_assert_int_eq(touchpad_config_set(dev->touchpad, &error,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_LEFT, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_RIGHT, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_TOP, -1,
					     TOUCHPAD_CONFIG_SOFTBUTTON_BOTTOM, 0,
					     TOUCHPAD_CONFIG_NONE), 3);
	ck_assert_int_eq(error, TOUCHPAD_CONFIG_ERROR_VALUE_TOO_LOW);

	ck_assert_int_eq(touchpad_config_set(dev->touchpad, &error,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_LEFT, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_RIGHT, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_TOP, 0,
					     TOUCHPAD_CONFIG_SOFTBUTTON_BOTTOM, -1,
					     TOUCHPAD_CONFIG_NONE), 4);
	ck_assert_int_eq(error, TOUCHPAD_CONFIG_ERROR_VALUE_TOO_LOW);
}
END_TEST

START_TEST(config_buttons_set_get)
{
	struct tptest_device *dev = tptest_current_device();
	enum touchpad_config_error error;
	int values[4];

	ck_assert_int_eq(touchpad_config_set(dev->touchpad, &error,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_LEFT, 30,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_RIGHT, 70,
					     TOUCHPAD_CONFIG_SOFTBUTTON_TOP, 40,
					     TOUCHPAD_CONFIG_SOFTBUTTON_BOTTOM, 90,
					     TOUCHPAD_CONFIG_NONE), 0);
	ck_assert_int_eq(error, TOUCHPAD_CONFIG_ERROR_NO_ERROR);
	ck_assert_int_eq(touchpad_config_get(dev->touchpad,
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_LEFT, &values[0],
					     TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_RIGHT, &values[1],
					     TOUCHPAD_CONFIG_SOFTBUTTON_TOP, &values[2],
					     TOUCHPAD_CONFIG_SOFTBUTTON_BOTTOM, &values[3],
					     TOUCHPAD_CONFIG_NONE), 0);
	ck_assert_int_eq(values[0], 30);
	ck_assert_int_eq(values[1], 70);
	ck_assert_int_eq(values[2], 40);
	ck_assert_int_eq(values[3], 90);
}
END_TEST

int main(void) {
	tptest_add("config_get", config_get, TOUCHPAD_ALL_DEVICES);
	tptest_add("config_get", config_get_invalid, TOUCHPAD_ALL_DEVICES);
	tptest_add("config_get", config_get_empty, TOUCHPAD_ALL_DEVICES);
	tptest_add("config_set", config_set_tap_enabled, TOUCHPAD_ALL_DEVICES);

	tptest_add("config_buttons", config_buttons_get_defaults, TOUCHPAD_ALL_DEVICES);
	tptest_add("config_buttons", config_buttons_set_invalid, TOUCHPAD_ALL_DEVICES);
	tptest_add("config_buttons", config_buttons_set_get, TOUCHPAD_ALL_DEVICES);
	return tptest_run();
}

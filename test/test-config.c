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

	struct tptest_device *dev = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);

	for (int i = TOUCHPAD_CONFIG_NONE + 1; i < TOUCHPAD_CONFIG_LAST; i++)
		ck_assert_int_eq(touchpad_config_get(dev->touchpad,
						     i, &value,
						     TOUCHPAD_CONFIG_NONE), 0);
	tptest_delete_device(dev);
}
END_TEST

START_TEST(config_get_invalid)
{
	int value;
	struct tptest_device *dev = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);

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
	tptest_delete_device(dev);
}
END_TEST

START_TEST(config_get_empty)
{
	int value = 10;
	struct tptest_device *dev = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);

	ck_assert_int_eq(touchpad_config_get(dev->touchpad, TOUCHPAD_CONFIG_NONE, &value), 0);
	ck_assert_int_eq(value, 10);
	tptest_delete_device(dev);
}
END_TEST

START_TEST(config_set_tap_enabled)
{
	enum touchpad_config_error error;
	struct tptest_device *dev = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
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
	tptest_delete_device(dev);
}
END_TEST

int main(void) {
	tptest_add("config", "config_get", config_get);
	tptest_add("config", "config_get", config_get_invalid);
	tptest_add("config", "config_get", config_get_empty);
	tptest_add("config", "config_set", config_set_tap_enabled);
	return tptest_run();
}

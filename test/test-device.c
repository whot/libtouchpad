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

#include "tptest.h"
#include "touchpad-util.h"
#include "touchpad-config.h"

START_TEST(device_open_invalid_path)
{
	int rc;
	struct touchpad *tp = NULL;

	rc = touchpad_new_from_path("/tmp", &tp);
	ck_assert(tp == NULL);
	ck_assert_int_eq(rc, -ENOTTY);
}
END_TEST

START_TEST(device_open_invalid_device)
{
	int rc;
	struct touchpad *tp = NULL;

	rc = touchpad_new_from_path("/dev/input/event0", &tp);
	ck_assert(tp == NULL);
	ck_assert_int_eq(rc, -ECANCELED);
}
END_TEST

START_TEST(device_change_fd)
{
	struct tptest_device *dev = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	int fd;

	fd = touchpad_get_fd(dev->touchpad);
	ck_assert_int_ge(fd, 0);

	ck_assert_int_eq(0, touchpad_change_fd(dev->touchpad, 0));
	ck_assert_int_eq(0, touchpad_get_fd(dev->touchpad));

	ck_assert_int_eq(0, touchpad_change_fd(dev->touchpad, -1));
	ck_assert_int_eq(-1, touchpad_get_fd(dev->touchpad));

	ck_assert_int_eq(0, touchpad_change_fd(dev->touchpad, fd));
	ck_assert_int_eq(fd, touchpad_get_fd(dev->touchpad));

	tptest_delete_device(dev);
}
END_TEST

START_TEST(device_reopen)
{
	struct tptest_device *dev = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);

	touchpad_close(dev->touchpad);
	ck_assert_int_eq(touchpad_reopen(dev->touchpad), 0);

	tptest_delete_device(dev);
}
END_TEST

int main(void) {
	tptest_add("device", "open", device_open_invalid_device);
	tptest_add("device", "open", device_open_invalid_path);
	tptest_add("device", "open", device_change_fd);
	tptest_add("device", "open", device_reopen);

	return tptest_run();
}

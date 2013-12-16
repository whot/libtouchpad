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
#include <fcntl.h>
#include <unistd.h>

#include "tptest.h"
#include "touchpad-util.h"
#include "touchpad-config.h"

START_TEST(device_open_invalid_device)
{
	int rc, fd;
	struct touchpad *tp = NULL;

	fd = open("/dev/input/event0", O_RDONLY);
	ck_assert_int_ge(fd, 0);
	rc = touchpad_new_from_fd(fd, &tp);
	ck_assert(tp == NULL);
	ck_assert_int_eq(rc, -ECANCELED);
	close(fd);
}
END_TEST

START_TEST(device_change_fd)
{
	struct tptest_device *dev = tptest_current_device();
	int fd;

	fd = touchpad_get_fd(dev->touchpad);
	ck_assert_int_ge(fd, 0);

	/* Can't add the fd onto itself */
	ck_assert_int_eq(-EINVAL, touchpad_change_fd(dev->touchpad, fd));
	ck_assert_int_eq(0, touchpad_change_fd(dev->touchpad, 0));
	/* changing the fd doesn't change the epoll fd */
	ck_assert_int_eq(fd, touchpad_get_fd(dev->touchpad));
}
END_TEST

int main(int argc, char **argv) {
	tptest_add("device_open", device_open_invalid_device, TOUCHPAD_NO_DEVICE);
	tptest_add("device_change_fd", device_change_fd, TOUCHPAD_ALL_DEVICES);

	return tptest_run(argc, argv);
}

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
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libevdev/libevdev.h>

#include "touchpad-int.h"
#include "touchpad-config.h"

void
touchpad_log(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

void touch_init(struct touchpad *tp, struct touch *t)
{
	t->x = t->y = 0;
	t->state = TOUCH_NONE;
	touchpad_history_reset(tp, t);
}

void
touchpad_reset(struct touchpad *tp)
{
	int i;

	touchpad_config_set_defaults(tp);
	for (i = 0; i < MAX_TOUCHPOINTS; i++)
		touch_init(tp, &tp->touches[i]);
	tp->slot = libevdev_get_current_slot(tp->dev);
	tp->tap.state = TAP_STATE_IDLE;
	tp->scroll.state = SCROLL_STATE_NONE;
}

struct touchpad*
touchpad_alloc(void)
{
	struct touchpad *tp = zalloc(sizeof(struct touchpad));

	if (tp) {
		tp->ntouches = 0;
		tp->dev = libevdev_new();
		touchpad_reset(tp);
	}
	return tp;
}

int
open_path(const char *path)
{
	int fd;

	fd = open(path, O_RDONLY|O_NONBLOCK);
	if (fd < 0)
		return -errno;
	return fd;
}

struct touchpad*
touchpad_new_from_path(const char *path)
{
	struct touchpad *tp;
	int fd, rc;
	int ntouches;

	assert(path);

	tp = touchpad_alloc();

	fd = open_path(path);
	if (fd < 0) {
		rc = fd;
		goto fail;
	}

	rc = libevdev_set_fd(tp->dev, fd);
	if (rc < 0)
		goto fail;

	ntouches = libevdev_get_num_slots(tp->dev);
	if (ntouches <= 0) {
		touchpad_log("This device does not support multitouch\n");
		rc = -ECANCELED;
		goto fail;
	}

	tp->ntouches = ntouches;
	tp->slot = libevdev_get_current_slot(tp->dev);
	tp->path = strdup(path);

	return tp;
fail:
	close(fd);
	touchpad_free(tp);
	touchpad_log("Failed to open %s:  %s", path, strerror(-rc));
	return NULL;
}

void
touchpad_free(struct touchpad *tp)
{
	if (!tp)
		return;

	libevdev_free(tp->dev);
	free(tp->path);
	free(tp);
}

int
touchpad_set_fd(struct touchpad *tp, int fd) {
	return libevdev_change_fd(tp->dev, fd);
}


int
touchpad_close(struct touchpad *tp)
{
	int fd;

	assert(tp);

	fd = libevdev_get_fd(tp->dev);
	if (fd > -1) {
		close(fd);
		libevdev_change_fd(tp->dev, -1);
		touchpad_reset(tp);
	}

	return 0;
}

int
touchpad_get_fd(struct touchpad *tp)
{
	return libevdev_get_fd(tp->dev);
}


int
touchpad_reopen(struct touchpad *tp)
{
	int fd;

	assert(tp);

	fd = libevdev_get_fd(tp->dev);
	if (fd > -1)
		return fd;

	fd = open_path(tp->path);
	if (fd < 0)
		return fd;

	if (touchpad_set_fd(tp, fd) < 0)
		return -EINVAL;

	touchpad_reset(tp);

	return touchpad_get_fd(tp);
}

int
touchpad_get_min_max(struct touchpad *tp, int axis, int *min, int *max, int *res)
{
	assert(tp);

	if (!libevdev_has_event_code(tp->dev, EV_ABS, axis))
			return -1;

	if (min)
		*min = libevdev_get_abs_minimum(tp->dev, axis);
	if (max)
		*max = libevdev_get_abs_maximum(tp->dev, axis);
	if (res)
		*res = libevdev_get_abs_resolution(tp->dev, axis);
	return 0;
}

void
touchpad_set_interface(struct touchpad *tp, const struct touchpad_interface *interface)
{
	assert(tp);
	assert(interface);

	tp->interface = interface;
}


struct libevdev*
touchpad_get_device(struct touchpad *tp)
{
	return tp->dev;
}

static int
touchpad_sync_device(struct touchpad *tp, void *userdata)
{
	int rc;
	struct input_event ev;

	rc = libevdev_next_event(tp->dev, LIBEVDEV_READ_FLAG_SYNC, &ev);

	/* fake a SYN_REPORT first */
	if (rc == LIBEVDEV_READ_STATUS_SYNC) {
		struct input_event syn = {
			.type = EV_SYN,
			.code = SYN_REPORT,
			.value = 0,
		};
		syn.time = ev.time;
		touchpad_handle_event(tp, userdata, &syn);
	}

	while(rc == LIBEVDEV_READ_STATUS_SYNC) {
		rc = libevdev_next_event(tp->dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
		touchpad_handle_event(tp, userdata, &ev);
	}

	return rc;
}

int
touchpad_handle_events(struct touchpad *tp, void *userdata)
{
	int rc = 0;
	enum libevdev_read_flag mode = LIBEVDEV_READ_FLAG_NORMAL;

	assert(tp);
	assert(tp->interface);

	do {
		struct input_event ev;
		rc = libevdev_next_event(tp->dev, mode, &ev);
		if (rc == LIBEVDEV_READ_STATUS_SYNC) {
			rc = touchpad_sync_device(tp, userdata);
			if (rc == -EAGAIN)
				rc = LIBEVDEV_READ_STATUS_SUCCESS;
		} else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
			touchpad_handle_event(tp, userdata, &ev);
		}
	} while (rc == LIBEVDEV_READ_STATUS_SUCCESS);

	return (rc != -EAGAIN) ? rc : 0;
}

int
touchpad_handle_timer_expired(struct touchpad *tp, unsigned int millis, void *userdata)
{
	touchpad_tap_handle_timeout(tp, millis, userdata);
	return 0;
}

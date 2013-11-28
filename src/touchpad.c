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


static void
default_log_func(struct touchpad *tp,
		 enum touchpad_log_priority priority,
		 void *data,
		 const char *format, va_list args)
{
	vprintf(format, args);
}

void
touchpad_log(struct touchpad *tp,
	     enum touchpad_log_priority priority,
	     const char *format, ...)
{
	va_list args;

	if (!tp->log.func)
		return;

	va_start(args, format);
	tp->log.func(tp, priority, tp->log.data, format, args);
	va_end(args);
}

void
touchpad_set_log_func(struct touchpad *tp, touchpad_log_func_t func, void *data)
{
	tp->log.func = func;
	tp->log.data = data;
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
		tp->log.func = default_log_func;
		tp->log.data = NULL;
		touchpad_config_set_defaults(tp);
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

int
touchpad_new_from_path(const char *path, struct touchpad **tp_out)
{
	struct touchpad *tp;
	int fd, rc;
	int ntouches;

	if (!path)
		return -EINVAL;

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
		rc = -ECANCELED;
		goto fail;
	}

	tp->ntouches = ntouches;
	tp->slot = libevdev_get_current_slot(tp->dev);
	tp->path = strdup(path);

	*tp_out = tp;
	return 0;
fail:
	close(fd);
	touchpad_free(tp);
	return rc;
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
touchpad_change_fd(struct touchpad *tp, int fd) {
	return libevdev_change_fd(tp->dev, fd);
}


int
touchpad_close(struct touchpad *tp)
{
	int fd;

	if (!argcheck_ptr_not_null(tp))
		return 0;

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

	if (!argcheck_ptr_not_null(tp))
		return -ENODEV;

	fd = libevdev_get_fd(tp->dev);
	if (fd > -1)
		return 0;

	fd = open_path(tp->path);
	if (fd < 0)
		return fd;

	if (touchpad_change_fd(tp, fd) < 0)
		return -EINVAL;

	touchpad_reset(tp);

	return 0;
}

int
touchpad_get_min_max(struct touchpad *tp, int axis, int *min, int *max, int *res)
{
	if (!argcheck_ptr_not_null(tp))
		return -1;

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
	if (!argcheck_ptr_not_null(interface))
		return;
	argcheck_ptr_not_null(interface->motion);
	argcheck_ptr_not_null(interface->button);
	argcheck_ptr_not_null(interface->scroll);
	argcheck_ptr_not_null(interface->tap);
	argcheck_ptr_not_null(interface->rotate);
	argcheck_ptr_not_null(interface->pinch);

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
touchpad_request_timer(struct touchpad *tp, void *userdata,
		       unsigned int now, unsigned int delta)
{
	int t = now + delta;

	if (delta == 0)
		return 0;

	if (tp->next_timeout)
		tp->next_timeout = min(t, tp->next_timeout);
	else
		tp->next_timeout = t;

	tp->interface->register_timer(tp, userdata, now, delta);
	return 0;
}

static int
touchpad_handle_timeouts(struct touchpad *tp, void *userdata, unsigned int now)
{
	if (tp->next_timeout == 0 || tp->next_timeout > now)
		return 0;

	tp->next_timeout = touchpad_tap_handle_timeout(tp, now, userdata);
	argcheck_int_ge(tp->next_timeout - now, 0);

	return 0;
}

int
touchpad_handle_events(struct touchpad *tp, void *userdata, unsigned int now)
{
	int rc = 0;
	enum libevdev_read_flag mode = LIBEVDEV_READ_FLAG_NORMAL;

	argcheck_ptr_not_null(tp->interface);

	do {
		struct input_event ev;
		rc = libevdev_next_event(tp->dev, mode, &ev);
		if (rc == LIBEVDEV_READ_STATUS_SYNC) {
			rc = touchpad_sync_device(tp, userdata);
			if (rc == -EAGAIN)
				rc = LIBEVDEV_READ_STATUS_SUCCESS;
		} else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
			if (ev.type == EV_SYN)
				touchpad_handle_timeouts(tp, userdata, timeval_to_millis(&ev.time));
			touchpad_handle_event(tp, userdata, &ev);
		} else if (rc == -EAGAIN)
			touchpad_handle_timeouts(tp, userdata, now);

	} while (rc == LIBEVDEV_READ_STATUS_SUCCESS);

	return (rc != -EAGAIN) ? rc : 0;
}

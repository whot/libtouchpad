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
#include <time.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <libevdev/libevdev.h>

#include "touchpad-int.h"
#include "touchpad-config.h"

static touchpad_error_log_func_t error_log_func;

void
touchpad_error_log(const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	if (error_log_func)
		error_log_func(msg, args);
	else
		vfprintf(stderr, msg, args);
	va_end(args);
}

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

void
touchpad_set_error_log_func(touchpad_error_log_func_t func)
{
	error_log_func = func;
}

void
touch_init(struct touchpad *tp, struct touch *t)
{
	t->x = t->y = 0;
	t->state = TOUCH_NONE;
	t->button_state = BUTTON_STATE_NONE;
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
		tp->buttons.handle_state = touchpad_button_handle_state;
		tp->buttons.handle_timeout = touchpad_button_handle_timeout;
		tp->buttons.select_pointer_touch = touchpad_button_select_pointer_touch;
		touchpad_config_set_static_defaults(tp);
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

static int
init_epollfd(struct touchpad *tp)
{
	int fd;
	int timerfd;
	struct epoll_event ev;

	fd = epoll_create1(O_CLOEXEC);

	if (fd < 0)
		goto fail;

	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLIN,
	ev.data.fd = libevdev_get_fd(tp->dev);
	if (epoll_ctl(fd, EPOLL_CTL_ADD, ev.data.fd, &ev) < 0)
		goto fail;

	timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC|TFD_NONBLOCK);
	if (timerfd < -1)
		goto fail;

	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLIN,
	ev.data.fd = timerfd;
	if (epoll_ctl(fd, EPOLL_CTL_ADD, timerfd, &ev) < 0)
		goto fail;

	tp->timerfd = timerfd;

	return fd;
fail:
	close(fd);
	close(timerfd);
	tp->timerfd = -1;
	tp->epollfd = -1;
	return -errno;

}

int
touchpad_new_from_fd(int fd, struct touchpad **tp_out)
{
	struct touchpad *tp;
	int rc;
	int ntouches;

	if (fd < 0)
		return -EBADF;

	tp = touchpad_alloc();

	rc = libevdev_set_fd(tp->dev, fd);
	if (rc < 0)
		goto fail;

	rc = libevdev_set_clock_id(tp->dev, CLOCK_MONOTONIC);
	if (rc < 0)
		goto fail;

	ntouches = libevdev_get_num_slots(tp->dev);
	if (ntouches <= 0) {
		rc = -ECANCELED;
		goto fail;
	}

	tp->epollfd = init_epollfd(tp);
	if (tp->epollfd < 0) {
		rc = tp->epollfd;
		goto fail;
	}

	tp->maxtouches = min(ntouches, MAX_TOUCHPOINTS);
	tp->slot = libevdev_get_current_slot(tp->dev);

	tp->ntouches = tp->maxtouches;
	if (libevdev_has_event_code(tp->dev, EV_KEY, BTN_TOOL_QUADTAP))
		tp->ntouches = max(4, tp->ntouches);
	if (libevdev_has_event_code(tp->dev, EV_KEY, BTN_TOOL_TRIPLETAP))
		tp->ntouches = max(3, tp->ntouches);
	if (libevdev_has_event_code(tp->dev, EV_KEY, BTN_TOOL_DOUBLETAP))
		tp->ntouches = max(2, tp->ntouches);

	if (libevdev_has_event_code(tp->dev, EV_KEY, BTN_RIGHT)) {
		tp->buttons.handle_state = touchpad_phys_button_handle_state;
		tp->buttons.handle_timeout = touchpad_phys_button_handle_timeout;
		tp->buttons.select_pointer_touch = touchpad_phys_button_select_pointer_touch;
	}

	touchpad_config_set_dynamic_defaults(tp);

	*tp_out = tp;
	return 0;
fail:
	touchpad_free(tp);
	return rc;
}

void
touchpad_free(struct touchpad *tp)
{
	if (!tp)
		return;

	libevdev_free(tp->dev);
	close(tp->epollfd);
	free(tp);
}

int
touchpad_change_fd(struct touchpad *tp, int fd) {
	int rc;
	struct epoll_event ev;

	epoll_ctl(tp->epollfd, EPOLL_CTL_DEL, libevdev_get_fd(tp->dev), NULL);

	rc = libevdev_change_fd(tp->dev, fd);
	if (rc == 0)
		touchpad_reset(tp);

	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	if (epoll_ctl(tp->epollfd, EPOLL_CTL_ADD, fd, &ev) < 0)
		return -errno;

	return rc;
}

int
touchpad_get_fd(struct touchpad *tp)
{
	return tp->epollfd;
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
	struct itimerspec its;

	if (delta == 0)
		return 0;

	if (tp->next_timeout)
		tp->next_timeout = min(t, tp->next_timeout);
	else
		tp->next_timeout = t;

	its.it_value.tv_sec = t/1000;
	its.it_value.tv_nsec = (t % 1000) * 1000 * 1000;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;

	timerfd_settime(tp->timerfd, TFD_TIMER_ABSTIME, &its, NULL);
	return 0;
}

static int
touchpad_handle_timeouts(struct touchpad *tp, void *userdata, unsigned int now)
{
	int timeout;
	int next_timeout = INT_MAX;
	struct timespec ts;

	if (now == 0) {
		clock_gettime(CLOCK_MONOTONIC, &ts);
		now = timespec_to_millis(&ts);
	}

	if (tp->next_timeout == 0 || tp->next_timeout > now)
		return 0;

	timeout = tp->buttons.handle_timeout(tp, now, userdata);
	if (timeout)
		next_timeout = timeout;

	timeout = touchpad_tap_handle_timeout(tp, now, userdata);
	if (timeout)
		next_timeout = min(timeout, next_timeout);

	tp->next_timeout = (next_timeout == INT_MAX) ? 0 : next_timeout;
	if (tp->next_timeout)
		argcheck_uint_ge(tp->next_timeout, now);

	return 0;
}

static void
touchpad_drain_timer_events(struct touchpad *tp)
{
	uint64_t buf;
	read(tp->timerfd, &buf, sizeof(buf));
}

int
touchpad_handle_events(struct touchpad *tp, void *userdata)
{
	int rc = 0;
	enum libevdev_read_flag mode = LIBEVDEV_READ_FLAG_NORMAL;
	struct epoll_event events[3];

	argcheck_ptr_not_null(tp->interface);


	rc = epoll_wait(tp->epollfd, events, ARRAY_LENGTH(events), 0);
	if (rc < 0)
		return -errno;
	else if (rc == 0)
		return touchpad_handle_timeouts(tp, userdata, 0);
	else {
		int i;
		for (i = 0; i < rc; i++) {
			if (events[i].data.fd == tp->timerfd) {
				touchpad_drain_timer_events(tp);
				break;
			}
		}
	}

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
			touchpad_handle_timeouts(tp, userdata, 0);

	} while (rc == LIBEVDEV_READ_STATUS_SUCCESS);

	return (rc != -EAGAIN) ? rc : 0;
}

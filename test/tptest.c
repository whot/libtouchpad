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
#include "touchpad-config.h"
#include "touchpad-util.h"
#include <check.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <ccan/list/list.h>
#include <linux/input.h>
#include <sys/ptrace.h>
#include <sys/timerfd.h>
#include <sys/wait.h>

#define MAX_SUITES 100

static int in_debugger = -1;

struct test {
	struct list_node node;
	char *name;
	TCase *tc;
};

struct suite {
	struct list_node node;
	struct list_head tests;
	char *name;
	Suite *suite;
};

static struct list_head all_tests = LIST_HEAD_INIT(all_tests);

static void
tptest_add_tcase(struct suite *suite, const char *name, void *func)
{
	struct test *t;
	list_for_each(&suite->tests, t, node) {
		if (strcmp(t->name, name) != 0)
			continue;

		tcase_add_test(t->tc, func);
		return;
	}

	t = zalloc(sizeof(*t));
	t->name = strdup(name);
	t->tc = tcase_create(name);
	list_add_tail(&suite->tests, &t->node);
	tcase_add_test(t->tc, func);
	suite_add_tcase(suite->suite, t->tc);
}

void
tptest_add(const char *suite, const char *name, void *func)
{
	struct suite *s;

	list_for_each(&all_tests, s, node) {
		if (strcmp(s->name, suite) == 0) {
			tptest_add_tcase(s, name, func);
			return;
		}
	}

	s = zalloc(sizeof(*s));
	s->name = strdup(suite);
	s->suite = suite_create(s->name);
	list_head_init(&s->tests);
	list_add_tail(&all_tests, &s->node);
	tptest_add_tcase(s, name, func);
}

int is_debugger_attached()
{
	int status;
	int rc;
	int pid = fork();

	if (pid == -1)
		return 0;

	if (pid == 0) {
		int ppid = getppid();
		if (ptrace(PTRACE_ATTACH, ppid, NULL, NULL) == 0) {
			waitpid(ppid, NULL, 0);
			ptrace(PTRACE_CONT, NULL, NULL);
			ptrace(PTRACE_DETACH, ppid, NULL, NULL);
			rc = 0;
		} else
			rc = 1;
		_exit(rc);
	} else {
		waitpid(pid, &status, 0);
		rc = WEXITSTATUS(status);
	}

	return rc;
}


int
tptest_run(void) {
	struct suite *s, *next;
	int failed;
	SRunner *sr = NULL;

	if (in_debugger == -1) {
		in_debugger = is_debugger_attached();
		if (in_debugger)
			setenv("CK_FORK", "no", 0);
	}

	list_for_each(&all_tests, s, node) {
		if (!sr)
			sr = srunner_create(s->suite);
		else
			srunner_add_suite(sr, s->suite);
	}

	srunner_run_all(sr, CK_NORMAL);
	failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	list_for_each_safe(&all_tests, s, next, node) {
		struct test *t, *tnext;

		list_for_each_safe(&s->tests, t, tnext, node) {
			free(t->name);
			list_del(&t->node);
			free(t);
		}

		list_del(&s->node);
		free(s->name);
		free(s);
	}

	return failed;
}

static void
tptest_create_synaptics_clickpad(struct tptest_device *d)
{
	struct libevdev *dev;
	struct input_absinfo abs[] = {
		{ ABS_X, 1472, 5472, 75 },
		{ ABS_Y, 1408, 4448, 129 },
		{ ABS_PRESSURE, 0, 255, 0 },
		{ ABS_TOOL_WIDTH, 0, 15, 0 },
		{ ABS_MT_SLOT, 0, 1, 0 },
		{ ABS_MT_POSITION_X, 1472, 5472, 75 },
		{ ABS_MT_POSITION_Y, 1408, 4448, 129 },
		{ ABS_MT_TRACKING_ID, 0, 65535, 0 },
		{ ABS_MT_PRESSURE, 0, 255, 0 }
	};
	struct input_absinfo *a;
	int rc;

	dev = libevdev_new();
	ck_assert(dev != NULL);

	libevdev_set_name(dev, "SynPS/2 Synaptics TouchPad");
	libevdev_set_id_bustype(dev, 0x11);
	libevdev_set_id_vendor(dev, 0x2);
	libevdev_set_id_product(dev, 0x11);
	libevdev_enable_event_code(dev, EV_KEY, BTN_LEFT, NULL);
	libevdev_enable_event_code(dev, EV_KEY, BTN_TOOL_FINGER, NULL);
	libevdev_enable_event_code(dev, EV_KEY, BTN_TOOL_QUINTTAP, NULL);
	libevdev_enable_event_code(dev, EV_KEY, BTN_TOUCH, NULL);
	libevdev_enable_event_code(dev, EV_KEY, BTN_TOOL_DOUBLETAP, NULL);
	libevdev_enable_event_code(dev, EV_KEY, BTN_TOOL_TRIPLETAP, NULL);
	libevdev_enable_event_code(dev, EV_KEY, BTN_TOOL_QUADTAP, NULL);

	ARRAY_FOR_EACH(abs, a)
		libevdev_enable_event_code(dev, EV_ABS, a->value, a);

	rc = libevdev_uinput_create_from_device(dev,
						LIBEVDEV_UINPUT_OPEN_MANAGED,
						&d->uinput);
	ck_assert_int_eq(rc, 0);
	libevdev_free(dev);
}

static void
push_event(struct tptest_device *d, const union tptest_event *e)
{
	union tptest_event *event = &d->events[d->idx++];
	*event = *e;
}

static void
motion(struct touchpad *t, void *userdata, int x, int y)
{
	struct tptest_device *d = userdata;
	union tptest_event e = { .motion.type = EVTYPE_MOTION,
				 .motion.x = x,
				 .motion.y = y};
	push_event(d, &e);
}

static void
button(struct touchpad *t, void *userdata, unsigned int button, bool is_press)
{
	struct tptest_device *d = userdata;
	union tptest_event e = { .button.type = EVTYPE_BUTTON,
				 .button.button = button,
				 .button.is_press = is_press };
	push_event(d, &e);
}

static void
tap(struct touchpad *t, void *userdata, unsigned int fingers, bool is_press)
{
	struct tptest_device *d = userdata;
	union tptest_event e = { .tap.type = EVTYPE_TAP,
				 .tap.fingers = fingers,
				 .tap.is_press = is_press };
	push_event(d, &e);
}

static void
scroll(struct touchpad *t, void *userdata, enum touchpad_scroll_direction dir, double units)
{
	struct tptest_device *d = userdata;
	union tptest_event e = { .scroll.type = EVTYPE_SCROLL,
				  .scroll.dir = dir,
				  .scroll.units = units };
	push_event(d, &e);
}

static int
register_timer(struct touchpad *tp, void *userdata, unsigned int now, unsigned int ms)
{
	int rc;
	struct tptest_device *d = userdata;
	struct itimerspec t;

	t.it_value.tv_sec = ms/1000;
	t.it_value.tv_nsec = (ms % 1000) * 1000 * 1000;
	t.it_interval.tv_sec = 0;
	t.it_interval.tv_nsec = 0;

	rc = timerfd_settime(d->timerfd, 0, &t, NULL);

	if (ms)
		d->latest_timer = max(now + ms, d->latest_timer);
	else
		d->latest_timer = 0;

	return rc < 0 ? -errno : 0;
}

static void
rotate(struct touchpad *tp, void *userdata, int degrees)
{
	argcheck_not_reached();
}

static void
pinch(struct touchpad *tp, void *userdata, int scale)
{
	argcheck_not_reached();
}

static const struct touchpad_interface interface = {
	.motion = motion,
	.button = button,
	.tap = tap,
	.scroll = scroll,
	.rotate = rotate,
	.pinch = pinch,
	.register_timer = register_timer,
};

static bool errors_allowed = false;

static void
error_log(const char *format, va_list args)
{
	vfprintf(stderr, format, args);
	if (!errors_allowed)
		ck_abort_msg("error function hit when errors are not allowed\n");
}

void
tptest_error(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	error_log(msg, args);
	va_end(args);
}

void
tptest_allow_errors(bool allow)
{
	errors_allowed = allow;
}

struct tptest_device *
tptest_create_device(enum tptest_device_type which)
{
	struct tptest_device *d = zalloc(sizeof(*d));
	int fd;
	int rc;
	const char *path;

	touchpad_set_error_log_func(error_log);

	ck_assert(d != NULL);

	switch(which) {
		case TOUCHPAD_SYNAPTICS_CLICKPAD:
			tptest_create_synaptics_clickpad(d);
			break;
	}

	path = libevdev_uinput_get_devnode(d->uinput);
	ck_assert(path != NULL);
	fd = open(path, O_RDWR|O_NONBLOCK);
	ck_assert_int_ne(fd, -1);

	rc = libevdev_new_from_fd(fd, &d->evdev);
	ck_assert_int_eq(rc, 0);

	rc = touchpad_new_from_fd(fd, &d->touchpad);
	ck_assert_int_eq(rc, 0);
	touchpad_set_interface(d->touchpad, &interface);

	ck_assert(d->touchpad != NULL);

	d->timerfd = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC|TFD_NONBLOCK);
	ck_assert(d->timerfd > -1);

	return d;
}

int
tptest_handle_events(struct tptest_device *d)
{
	struct pollfd fds[2];

	fds[0].fd = touchpad_get_fd(d->touchpad);
	fds[0].events = POLLIN;
	fds[1].fd = d->timerfd;
	fds[1].events = POLLIN;

	while (poll(fds, ARRAY_LENGTH(fds), 1)) {
		struct timespec t;
		unsigned int now;

		clock_gettime(CLOCK_REALTIME, &t);
		now = t.tv_sec * 1000 + t.tv_nsec/1000000;
		if (now >= d->latest_timer)
			d->latest_timer = 0;
		touchpad_handle_events(d->touchpad, d, now);

		if (fds[1].revents) {
			uint64_t buf;
			read(fds[1].fd, &buf, sizeof(buf));
		}
	}

	return d->latest_timer != 0;
}

void
tptest_delete_device(struct tptest_device *d)
{
	if (!d)
		return;

	touchpad_free(d->touchpad);
	libevdev_free(d->evdev);
	libevdev_uinput_destroy(d->uinput);
	close(d->timerfd);
	memset(d,0, sizeof(*d));
	free(d);
}

void
tptest_event(struct tptest_device *d, unsigned int type, unsigned int code, int value)
{
	libevdev_uinput_write_event(d->uinput, type, code, value);
}

void
tptest_touch_down(struct tptest_device *d, unsigned int slot, int x, int y)
{
	static int tracking_id;
	struct input_event *ev;
	struct input_event down[] = {
		{ .type = EV_ABS, .code = ABS_X, .value = x  },
		{ .type = EV_ABS, .code = ABS_Y, .value = y },
		{ .type = EV_ABS, .code = ABS_PRESSURE, .value = 30  },
		{ .type = EV_ABS, .code = ABS_MT_SLOT, .value = slot },
		{ .type = EV_ABS, .code = ABS_MT_TRACKING_ID, .value = ++tracking_id },
		{ .type = EV_ABS, .code = ABS_MT_POSITION_X, .value = x },
		{ .type = EV_ABS, .code = ABS_MT_POSITION_Y, .value = y },
		{ .type = EV_SYN, .code = SYN_REPORT, .value = 0 },
	};

	ARRAY_FOR_EACH(down, ev)
		tptest_event(d, ev->type, ev->code, ev->value);
}

void
tptest_touch_up(struct tptest_device *d, unsigned int slot)
{
	struct input_event *ev;
	struct input_event up[] = {
		{ .type = EV_ABS, .code = ABS_MT_SLOT, .value = slot },
		{ .type = EV_ABS, .code = ABS_MT_TRACKING_ID, .value = -1 },
		{ .type = EV_SYN, .code = SYN_REPORT, .value = 0 },
	};

	ARRAY_FOR_EACH(up, ev)
		tptest_event(d, ev->type, ev->code, ev->value);
}

void
tptest_touch_move(struct tptest_device *d, unsigned int slot, int x, int y)
{
	struct input_event *ev;
	struct input_event move[] = {
		{ .type = EV_ABS, .code = ABS_MT_SLOT, .value = slot },
		{ .type = EV_ABS, .code = ABS_X, .value = x  },
		{ .type = EV_ABS, .code = ABS_Y, .value = y },
		{ .type = EV_ABS, .code = ABS_MT_POSITION_X, .value = x },
		{ .type = EV_ABS, .code = ABS_MT_POSITION_Y, .value = y },
		{ .type = EV_KEY, .code = BTN_TOOL_FINGER, .value = 1 },
		{ .type = EV_KEY, .code = BTN_TOUCH, .value = 1 },
		{ .type = EV_SYN, .code = SYN_REPORT, .value = 0 },
	};

	ARRAY_FOR_EACH(move, ev)
		tptest_event(d, ev->type, ev->code, ev->value);
}

void
tptest_touch_move_to(struct tptest_device *d, unsigned int slot, int x_from, int y_from, int x_to, int y_to, int steps)
{
	if (steps == -1)
		touchpad_config_get(d->touchpad, TOUCHPAD_CONFIG_MOTION_HISTORY_SIZE, &steps,
						 TOUCHPAD_CONFIG_NONE);

	for (int i = 0; i < steps - 1; i++)
		tptest_touch_move(d, slot, x_from + (x_to - x_from)/steps * i, y_from + (y_to - y_from)/steps * i);
	tptest_touch_move(d, slot, x_to, y_to);
}

void
tptest_click(struct tptest_device *d, bool is_press)
{

	struct input_event *ev;
	struct input_event click[] = {
		{ .type = EV_KEY, .code = BTN_LEFT, .value = is_press ? 1 : 0 },
		{ .type = EV_SYN, .code = SYN_REPORT, .value = 0 },
	};

	ARRAY_FOR_EACH(click, ev)
		tptest_event(d, ev->type, ev->code, ev->value);
}

struct tptest_button_event *tptest_button_event(union tptest_event *e)
{
	assert(e->type == EVTYPE_BUTTON);
	return &e->button;
}

struct tptest_motion_event *tptest_motion_event(union tptest_event *e)
{
	assert(e->type == EVTYPE_MOTION);
	return &e->motion;
}

struct tptest_tap_event *tptest_tap_event(union tptest_event *e)
{
	assert(e->type == EVTYPE_TAP);
	return &e->tap;
}

struct tptest_scroll_event *tptest_scroll_event(union tptest_event *e)
{
	assert(e->type == EVTYPE_SCROLL);
	return &e->scroll;
}

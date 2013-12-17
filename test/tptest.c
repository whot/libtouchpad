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
#include "tptest-int.h"
#include "tptest-synaptics.h"
#include "tptest-bcm5974.h"
#include "touchpad.h"
#include "touchpad-config.h"
#include "touchpad-util.h"
#include <check.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <ccan/list/list.h>
#include <linux/input.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#define MAX_SUITES 100

static int in_debugger = -1;

struct test {
	struct list_node node;
	char *name;
	TCase *tc;
	enum tptest_device_type devices;
};

struct suite {
	struct list_node node;
	struct list_head tests;
	char *name;
	Suite *suite;
};

static struct tptest_device *current_device;

struct tptest_device *tptest_current_device(void) {
	return current_device;
}

void tptest_set_current_device(struct tptest_device *device) {
	current_device = device;
}

static void generic_device_teardown(void)
{
	tptest_delete_device(current_device);
	current_device = NULL;
}

struct device devices[] = {
	{
		.type = TOUCHPAD_SYNAPTICS_CLICKPAD,
		.shortname = "synaptics",
		.setup = tptest_synaptics_clickpad_setup,
		.teardown = generic_device_teardown,
		.create = tptest_create_synaptics_clickpad,
		.touch_down = tptest_synaptics_clickpad_touch_down,
		.move = tptest_synaptics_clickpad_move,
	},
	{
		.type = TOUCHPAD_BCM5974,
		.shortname = "bcm5974",
		.setup = tptest_bcm5974_setup,
		.teardown = generic_device_teardown,
		.create = tptest_create_bcm5974,
		.touch_down = tptest_bcm5974_touch_down,
		.move = tptest_bcm5974_move,
	},
	{ TOUCHPAD_NO_DEVICE, "no device", NULL, NULL },
};


static struct list_head all_tests = LIST_HEAD_INIT(all_tests);

const struct device*
lookup_device(enum tptest_device_type type)
{
	struct device *d = devices;
	while (d->type != TOUCHPAD_NO_DEVICE) {
		if (d->type == type)
			return d;
		d++;
	}
	return d;
}

static void
tptest_add_tcase_for_device(struct suite *suite, void *func, enum tptest_device_type device)
{
	struct test *t;
	const struct device *dev = lookup_device(device);
	const char *test_name = dev->shortname;

	list_for_each(&suite->tests, t, node) {
		if (strcmp(t->name, test_name) != 0)
			continue;

		tcase_add_test(t->tc, func);
		return;
	}

	t = zalloc(sizeof(*t));
	t->name = strdup(test_name);
	t->tc = tcase_create(test_name);
	list_add_tail(&suite->tests, &t->node);
	if (device != TOUCHPAD_NO_DEVICE)
		tcase_add_checked_fixture(t->tc, dev->setup, dev->teardown);
	tcase_add_test(t->tc, func);
	suite_add_tcase(suite->suite, t->tc);
}

static void
tptest_add_tcase(struct suite *suite, void *func, enum tptest_device_type devices)
{
	if (devices != TOUCHPAD_NO_DEVICE) {
		enum tptest_device_type mask = TOUCHPAD_SYNAPTICS_CLICKPAD;
		while (mask <= devices) {
			if (devices & mask)
				tptest_add_tcase_for_device(suite, func, mask);
			mask <<= 1;
		}
	} else
		tptest_add_tcase_for_device(suite, func, TOUCHPAD_NO_DEVICE);
}

void
tptest_add(const char *name, void *func, enum tptest_device_type devices)
{
	struct suite *s;

	list_for_each(&all_tests, s, node) {
		if (strcmp(s->name, name) == 0) {
			tptest_add_tcase(s, func, devices);
			return;
		}
	}

	s = zalloc(sizeof(*s));
	s->name = strdup(name);
	s->suite = suite_create(s->name);
	list_head_init(&s->tests);
	list_add_tail(&all_tests, &s->node);
	tptest_add_tcase(s, func, devices);
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


static void
tptest_list_tests(struct list_head *tests)
{
	struct suite *s;

	list_for_each(tests, s, node) {
		struct test *t;
		printf("%s:\n", s->name);
		list_for_each(&s->tests, t, node) {
			printf("	%s\n", t->name);
		}
	}
}

static const struct option opts[] = {
	{ "list", 0, 0, 'l' },
	{ 0, 0, 0, 0}
};

int
tptest_run(int argc, char **argv) {
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

	while(1) {
		int c;
		int option_index = 0;

		c = getopt_long(argc, argv, "", opts, &option_index);
		if (c == -1)
			break;
		switch(c) {
			case 'l':
				tptest_list_tests(&all_tests);
				return 0;
		}
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
	struct device *dev;

	touchpad_set_error_log_func(error_log);

	ck_assert(d != NULL);

	dev = devices;
	while (dev->type != TOUCHPAD_NO_DEVICE) {
		if (dev->type == which) {
			dev->create(d);
			break;
		}
		dev++;
	}

	if (dev->type == TOUCHPAD_NO_DEVICE) {
		ck_abort_msg("Invalid device type %d\n", which);
		return NULL;
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

	d->d = dev;
	d->d->min[ABS_X] = libevdev_get_abs_minimum(d->evdev, ABS_X);
	d->d->max[ABS_X] = libevdev_get_abs_maximum(d->evdev, ABS_X);
	d->d->min[ABS_Y] = libevdev_get_abs_minimum(d->evdev, ABS_Y);
	d->d->max[ABS_Y] = libevdev_get_abs_maximum(d->evdev, ABS_Y);
	return d;
}

int
tptest_handle_events(struct tptest_device *d)
{
	return touchpad_handle_events(d->touchpad, d);
}

void
tptest_delete_device(struct tptest_device *d)
{
	if (!d)
		return;

	touchpad_free(d->touchpad);
	libevdev_free(d->evdev);
	libevdev_uinput_destroy(d->uinput);
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
	d->d->touch_down(d, slot, x, y);
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
	d->d->move(d, slot, x, y);
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

int tptest_scale(const struct tptest_device *d, unsigned int axis, int val)
{
	ck_assert_int_ge(val, 0);
	ck_assert_int_le(val, 100);
	ck_assert_int_le(axis, ABS_Y);

	return (d->d->max[axis] - d->d->min[axis]) * val/100.0 + d->d->min[axis];
}


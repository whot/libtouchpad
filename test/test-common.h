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

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <check.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include <touchpad.h>
#endif

enum device_type {
	TOUCHPAD_SYNAPTICS_CLICKPAD,
};

enum event_type {
	EVTYPE_NONE = 0,
	EVTYPE_MOTION,
	EVTYPE_BUTTON,
	EVTYPE_TAP,
};

struct event {
	enum event_type type;
	int x, y; /* for motion */
	unsigned button; /* for tap/button */
	bool is_press; /* for tap/button */
	double unit; /* for scroll */
};

struct device {
	struct libevdev *evdev;
	struct libevdev_uinput *uinput;
	struct touchpad *touchpad;
	struct event events[100];
	size_t idx;

	int timerfd;
	unsigned int latest_timer;
};

void test_common_add(const char *suite, const char *name, void *func);
int test_common_run(void);
struct device * test_common_create_device(enum device_type which);
void test_common_delete_device(struct device *d);
int test_common_handle_events(struct device *d);

void test_common_touch_up(struct device *d, unsigned int slot);
void test_common_touch_move(struct device *d, unsigned int slot, int x, int y);
void test_common_touch_down(struct device *d, unsigned int slot, int x, int y);
void test_common_touch_move_to(struct device *d, unsigned int slot, int x_from, int y_from, int x_to, int y_to, int steps);

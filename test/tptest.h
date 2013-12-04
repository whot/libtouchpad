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

#ifndef TPTEST_H
#define TPTEST_H

#include <check.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include <touchpad.h>

enum tptest_device_type {
	TOUCHPAD_NO_DEVICE = 0,
	TOUCHPAD_SYNAPTICS_CLICKPAD = 0x1,

	TOUCHPAD_ALL_DEVICES = TOUCHPAD_SYNAPTICS_CLICKPAD,
};

enum tptest_event_type {
	EVTYPE_NONE = 0,
	EVTYPE_MOTION,
	EVTYPE_BUTTON,
	EVTYPE_TAP,
	EVTYPE_SCROLL,
};

struct tptest_motion_event {
	enum tptest_event_type type;
	int x, y;
};

struct tptest_button_event {
	enum tptest_event_type type;
	unsigned button;
	bool is_press;
};

struct tptest_tap_event {
	enum tptest_event_type type;
	unsigned fingers;
	bool is_press;
};

struct tptest_scroll_event {
	enum tptest_event_type type;
	double units;
	enum touchpad_scroll_direction dir;
};

union tptest_event {
	enum tptest_event_type type;
	struct tptest_motion_event motion;
	struct tptest_button_event button;
	struct tptest_tap_event tap;
	struct tptest_scroll_event scroll;
};

struct tptest_device {
	struct libevdev *evdev;
	struct libevdev_uinput *uinput;
	struct touchpad *touchpad;
	union tptest_event events[100];
	size_t idx;

	int timerfd;
	unsigned int latest_timer;

	struct device *d;
};

void tptest_add(const char *name, void *func, enum tptest_device_type devices);
int tptest_run(int argc, char **argv);
struct tptest_device * tptest_create_device(enum tptest_device_type which);
struct tptest_device *tptest_current_device(void);
void tptest_delete_device(struct tptest_device *d);
int tptest_handle_events(struct tptest_device *d);

void tptest_event(struct tptest_device *t, unsigned int type, unsigned int code, int value);
void tptest_touch_up(struct tptest_device *d, unsigned int slot);
void tptest_touch_move(struct tptest_device *d, unsigned int slot, int x, int y);
void tptest_touch_down(struct tptest_device *d, unsigned int slot, int x, int y);
void tptest_touch_move_to(struct tptest_device *d, unsigned int slot, int x_from, int y_from, int x_to, int y_to, int steps);
void tptest_click(struct tptest_device *d, bool is_press);

struct tptest_button_event *tptest_button_event(union tptest_event *e);
struct tptest_motion_event *tptest_motion_event(union tptest_event *e);
struct tptest_tap_event *tptest_tap_event(union tptest_event *e);
struct tptest_scroll_event *tptest_scroll_event(union tptest_event *e);
void tptest_error(const char *msg, ...);
#define argcheck_log(_file, _line, _func, msg, ...)  \
	tptest_error("%s:%d %s(): " msg, _file, _line, _func, ## __VA_ARGS__)
#include <ccan/argcheck/argcheck.h>

#endif /* TPTEST_H */

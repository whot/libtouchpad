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
#include "touchpad-util.h"

void
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

void tptest_synaptics_clickpad_setup(void)
{
	struct tptest_device *d = tptest_create_device(TOUCHPAD_SYNAPTICS_CLICKPAD);
	tptest_set_current_device(d);
}

void
tptest_synaptics_clickpad_touch_down(struct tptest_device *d, unsigned int slot, int x, int y)
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

	down[0].value = tptest_scale(d, ABS_X, x);
	down[1].value = tptest_scale(d, ABS_Y, y);
	down[5].value = tptest_scale(d, ABS_X, x);
	down[6].value = tptest_scale(d, ABS_Y, y);

	ARRAY_FOR_EACH(down, ev)
		tptest_event(d, ev->type, ev->code, ev->value);
}

void
tptest_synaptics_clickpad_move(struct tptest_device *d, unsigned int slot, int x, int y)
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

	move[1].value = tptest_scale(d, ABS_X, x);
	move[2].value = tptest_scale(d, ABS_Y, y);
	move[3].value = tptest_scale(d, ABS_X, x);
	move[4].value = tptest_scale(d, ABS_Y, y);

	ARRAY_FOR_EACH(move, ev)
		tptest_event(d, ev->type, ev->code, ev->value);
}


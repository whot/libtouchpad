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
tptest_create_synaptics_non_mt(struct tptest_device *d)
{
	struct libevdev *dev;
	struct input_absinfo abs[] = {
		{ ABS_X, 1472, 5472, 93 },
		{ ABS_Y, 1408, 4448, 125 },
		{ ABS_PRESSURE, 0, 255, 0 },
		{ ABS_TOOL_WIDTH, 0, 15, 0 },
	};
	struct input_absinfo *a;
	int rc;

	dev = libevdev_new();
	ck_assert(dev != NULL);

	libevdev_set_name(dev, "SynPS/2 Synaptics TouchPad");
	libevdev_set_id_bustype(dev, 0x11);
	libevdev_set_id_vendor(dev, 0x2);
	libevdev_set_id_product(dev, 0x7);
	libevdev_enable_event_code(dev, EV_KEY, BTN_LEFT, NULL);
	libevdev_enable_event_code(dev, EV_KEY, BTN_RIGHT, NULL);
	libevdev_enable_event_code(dev, EV_KEY, BTN_TOOL_FINGER, NULL);
	libevdev_enable_event_code(dev, EV_KEY, BTN_TOUCH, NULL);

	ARRAY_FOR_EACH(abs, a)
		libevdev_enable_event_code(dev, EV_ABS, a->value, a);

	rc = libevdev_uinput_create_from_device(dev,
						LIBEVDEV_UINPUT_OPEN_MANAGED,
						&d->uinput);
	ck_assert_int_eq(rc, 0);
	libevdev_free(dev);
}

void tptest_synaptics_non_mt_setup(void)
{
	struct tptest_device *d = tptest_create_device(TOUCHPAD_SYNAPTICS_NON_MT);
	tptest_set_current_device(d);
}

void
tptest_synaptics_non_mt_touch_down(struct tptest_device *d, unsigned int slot, int x, int y)
{
	struct input_event *ev;
	struct input_event down[] = {
		{ .type = EV_KEY, .code = BTN_TOUCH, .value = 1  },
		{ .type = EV_KEY, .code = BTN_TOOL_FINGER, .value = 1  },
		{ .type = EV_ABS, .code = ABS_X, .value = x  },
		{ .type = EV_ABS, .code = ABS_Y, .value = y },
		{ .type = EV_ABS, .code = ABS_PRESSURE, .value = 30  },
		{ .type = EV_SYN, .code = SYN_REPORT, .value = 0 },
	};


	ck_assert_int_eq(slot, 0);

	down[1].value = tptest_scale(d, ABS_X, x);
	down[2].value = tptest_scale(d, ABS_Y, y);

	ARRAY_FOR_EACH(down, ev)
		tptest_event(d, ev->type, ev->code, ev->value);
}

void
tptest_synaptics_non_mt_move(struct tptest_device *d, unsigned int slot, int x, int y)
{
	struct input_event *ev;
	struct input_event move[] = {
		{ .type = EV_ABS, .code = ABS_X, .value = x  },
		{ .type = EV_ABS, .code = ABS_Y, .value = y },
		{ .type = EV_KEY, .code = BTN_TOOL_FINGER, .value = 1 },
		{ .type = EV_KEY, .code = BTN_TOUCH, .value = 1 },
		{ .type = EV_SYN, .code = SYN_REPORT, .value = 0 },
	};

	ck_assert_int_eq(slot, 0);

	move[0].value = tptest_scale(d, ABS_X, x);
	move[1].value = tptest_scale(d, ABS_Y, y);

	ARRAY_FOR_EACH(move, ev)
		tptest_event(d, ev->type, ev->code, ev->value);
}


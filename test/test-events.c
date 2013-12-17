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
#include <assert.h>

#include "tptest.h"
#include "touchpad-util.h"
#include "touchpad-config.h"

START_TEST(events_EV_SYN_only)
{
	struct tptest_device *dev = tptest_current_device();

	tptest_event(dev, EV_SYN, SYN_REPORT, 0);

	while (tptest_handle_events(dev))
		;
}
END_TEST

START_TEST(events_ABS_MT_TRACKING_ID_finishes)
{
	struct tptest_device *dev = tptest_current_device();

	tptest_event(dev, EV_ABS, ABS_MT_TRACKING_ID, -1);
	tptest_event(dev, EV_SYN, SYN_REPORT, 0);

	while (tptest_handle_events(dev))
		;
}
END_TEST

START_TEST(events_touch_start_finish_same_event)
{
	struct tptest_device *dev = tptest_current_device();

	tptest_event(dev, EV_ABS, ABS_MT_SLOT, 0);
	tptest_event(dev, EV_ABS, ABS_MT_POSITION_X, 2000);
	tptest_event(dev, EV_ABS, ABS_MT_POSITION_Y, 2000);
	tptest_event(dev, EV_ABS, ABS_MT_TRACKING_ID, 1);
	tptest_event(dev, EV_ABS, ABS_MT_SLOT, 1);
	tptest_event(dev, EV_ABS, ABS_MT_TRACKING_ID, -1);
	tptest_event(dev, EV_SYN, SYN_REPORT, 0);

	while (tptest_handle_events(dev))
		;
}
END_TEST

START_TEST(events_exceed_max_touches)
{
	struct tptest_device *dev = tptest_current_device();
	int i;


	for (i = 0; i < libevdev_get_num_slots(dev->evdev); i++)
		tptest_touch_down(dev, i, 10, 10);

	tptest_handle_events(dev);

	for (i = 0; i < libevdev_get_num_slots(dev->evdev); i++)
		tptest_touch_move_to(dev, i, 10, 10, 90, 90, -1);

	for (i = 0; i < libevdev_get_num_slots(dev->evdev); i++)
		tptest_touch_up(dev, i);

	tptest_handle_events(dev);
}
END_TEST

int main(int argc, char **argv) {
	tptest_add("events_invalid_touches", events_EV_SYN_only, TOUCHPAD_ALL_DEVICES);
	tptest_add("events_invalid_touches", events_ABS_MT_TRACKING_ID_finishes, TOUCHPAD_ALL_DEVICES);
	tptest_add("events_invalid_touches", events_touch_start_finish_same_event, TOUCHPAD_ALL_DEVICES);

	tptest_add("events_max_touches", events_exceed_max_touches, TOUCHPAD_BCM5974);

	return tptest_run(argc, argv);
}

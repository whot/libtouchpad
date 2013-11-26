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
#include <stdlib.h>
#include "touchpad-int.h"

static int
hysteresis(int in, int center, int margin)
{
	int diff = in - center;

	if (abs(diff) <= margin)
		diff = 0;
	else if (diff > margin)
		diff -= margin;
	else if (diff < -margin)
		diff += margin;

	return center + diff;
}

static void
touchpad_hysteresis_filter_motion(struct touch *t, int *x, int *y)
{
	struct touch_history_point *old = touchpad_history_get_last(t);
	const int hysteresis_margin = 8;

	arg_require_not_null(old);

	*x = hysteresis(*x, old->x, hysteresis_margin);
	*y = hysteresis(*y, old->y, hysteresis_margin);
}

/**
 * Convert the touchpoint coordinates to deltas based on the history.
 * We have N points (current + history), add the most recent N/2, subtract
 * the previous N/2 coordinates. Then divide by N, gives us an average that
 * we use as the delta.
 *
 * For an uneven number of data points, just drop the last and pretend we
 * have an even number.
 */
void
touchpad_motion_to_delta(struct touch *t, int *dx_out, int *dy_out)
{
	int i;
	int dx = t->x,
	    dy = t->y;
	int npoints;

	if (t->history.valid < t->history.size) {
		*dx_out = 0;
		*dy_out = 0;
		return;
	}

	npoints = (1 + t->history.valid)/2 * 2;

	for (i = 1; i < npoints; i++) {
		struct touch_history_point *p = touchpad_history_get(t, i);

		if ((i + 1) <= npoints/2) {
			dx += p->x;
			dy += p->y;
		} else {
			dx -= p->x;
			dy -= p->y;
		}
	}

	dx /= npoints;
	dy /= npoints;

	*dx_out = dx;
	*dy_out = dy;
}

void
touchpad_motion_dejitter(struct touch *t)
{
	touchpad_hysteresis_filter_motion(t, &t->x, &t->y);
}

void
touchpad_history_reset(struct touchpad *tp, struct touch *t)
{
	t->history.index = 0;
	t->history.valid = 0;
	t->history.size = tp->config.motion_history_size;
}

void
touchpad_history_push(struct touch *t, int x, int y, unsigned int millis)
{
	int index = t->history.index;

	t->history.points[index].x = x;
	t->history.points[index].y = y;
	t->history.points[index].millis = millis;
	t->history.valid = min(t->history.valid + 1, t->history.size);
	t->history.index = (t->history.index + 1) % t->history.size;
}

struct touch_history_point *
touchpad_history_get_last(struct touch *t)
{
	return touchpad_history_get(t, -1);
}

struct touch_history_point *
touchpad_history_get(struct touch *t, int when)
{
	int index;
	assert(when != 0);

	when = abs(when);
	index = (((int)t->history.index - when) + t->history.size) % t->history.size;

	return when > t->history.valid ? NULL : &t->history.points[index];
}

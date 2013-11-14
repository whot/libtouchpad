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

#include "touchpad-int.h"

static double
touchpad_scroll_units(struct touchpad *tp, struct touch *t,
		      enum touchpad_scroll_direction direction)
{
	double delta, threshold;
	int dx, dy;

	touchpad_motion_to_delta(t, &dx, &dy);


	switch(direction) {
		case TOUCHPAD_SCROLL_VERTICAL:
			delta = dy;
			threshold = tp->scroll.config.vdelta;
			break;
		case TOUCHPAD_SCROLL_HORIZONTAL:
			delta = dx;
			threshold = tp->scroll.config.hdelta;
			break;
		default:
			log_bug(direction, "invalid scroll direction %d\n", direction);
			return false;
	}

	return delta/threshold;
}

static int
touchpad_scroll_handle_2fg(struct touchpad *tp, void *userdata,
			   enum touchpad_scroll_direction direction)
{
	int i;
	double delta = 0;

	if (tp->fingers_down != 2) {
		if (tp->scroll.state != SCROLL_STATE_NONE) {
			tp->scroll.state = SCROLL_STATE_NONE;
			tp->interface->scroll(tp, userdata, direction, 0);
			return 1;
		}
		return 0;
	}

	for (i = 0; i < tp->ntouches; i++) {
		struct touch *t = touchpad_touch(tp, i);

		if (!t->dirty || t->state != TOUCH_UPDATE)
			continue;

		delta = max(delta, touchpad_scroll_units(tp, t, direction));
	}

	/* require scroll dist for first scroll event */
	if (delta < 1.0 && tp->scroll.state == SCROLL_STATE_NONE) {
		delta = 0;
	} else if (delta) {
		tp->interface->scroll(tp, userdata, direction, delta);
		tp->scroll.state = SCROLL_STATE_SCROLLING;
		tp->scroll.direction = direction;
	}

	return tp->scroll.state == SCROLL_STATE_SCROLLING;
}

static int
touchpad_scroll_handle_edge(struct touchpad *tp, void *userdata,
			    enum touchpad_scroll_direction direction)
{
	return 0; /* FIXME some more calculation needed */
}

static int
touchpad_scroll_continue(struct touchpad *tp, void *userdata)
{
	switch (tp->scroll.direction) {
		case TOUCHPAD_SCROLL_VERTICAL:
			if (tp->scroll.config.methods & TOUCHPAD_SCROLL_TWOFINGER_VERTICAL)
				touchpad_scroll_handle_2fg(tp, userdata, tp->scroll.direction);
			else if (tp->scroll.config.methods & TOUCHPAD_SCROLL_EDGE_VERTICAL)
				touchpad_scroll_handle_edge(tp, userdata, TOUCHPAD_SCROLL_VERTICAL);
			break;
		case TOUCHPAD_SCROLL_HORIZONTAL:
			if (tp->scroll.config.methods & TOUCHPAD_SCROLL_TWOFINGER_HORIZONTAL)
				touchpad_scroll_handle_2fg(tp, userdata, tp->scroll.direction);
			else if (tp->scroll.config.methods & TOUCHPAD_SCROLL_EDGE_HORIZONTAL)
				touchpad_scroll_handle_edge(tp, userdata, TOUCHPAD_SCROLL_HORIZONTAL);
			break;
	}

	return 1;
}

int
touchpad_scroll_handle_state(struct touchpad *tp, void *userdata)
{
	int rc = 0;

	/* Can't two-finger scroll with a clickpad button down */
	if (tp->buttons.state != 0)
		return 0;

	if (tp->scroll.state != SCROLL_STATE_NONE)
		return touchpad_scroll_continue(tp, userdata);

	if (tp->scroll.config.methods & TOUCHPAD_SCROLL_TWOFINGER_VERTICAL)
		rc = touchpad_scroll_handle_2fg(tp, userdata, TOUCHPAD_SCROLL_VERTICAL);
	else if (tp->scroll.config.methods & TOUCHPAD_SCROLL_EDGE_VERTICAL)
		rc = touchpad_scroll_handle_edge(tp, userdata, TOUCHPAD_SCROLL_VERTICAL);

	if (rc)
		return rc;

	if (tp->scroll.config.methods & TOUCHPAD_SCROLL_TWOFINGER_HORIZONTAL)
		rc = touchpad_scroll_handle_2fg(tp, userdata, TOUCHPAD_SCROLL_HORIZONTAL);
	else if (tp->scroll.config.methods & TOUCHPAD_SCROLL_EDGE_HORIZONTAL)
		rc = touchpad_scroll_handle_edge(tp, userdata, TOUCHPAD_SCROLL_HORIZONTAL);

	return rc;

}

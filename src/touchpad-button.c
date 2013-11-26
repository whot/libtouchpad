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
#include <linux/input.h>
#include "touchpad-int.h"

static inline bool
is_inside_area(struct touch *t, int area[4]) {
	return (t->x >= area[0] && t->x <= area[1] &&
		t->y >= area[2] && t->y <= area[3]);
}

static int
guess_softbutton(struct touchpad *tp, void *userdata)
{

	struct touch *t;

	/* We assume that whatever we picked as pinned finger is the
	   finger that presses the button */
	t = touchpad_pinned_touch(tp);
	if (t && is_inside_area(t, tp->buttons.config.right))
		return BTN_RIGHT;

	return BTN_LEFT;
}

int
touchpad_button_handle_state(struct touchpad *tp, void *userdata)
{
	uint32_t current, old;

	if ((tp->queued & (EVENT_BUTTON_PRESS|EVENT_BUTTON_RELEASE)) == 0)
		return 0;

	current = tp->buttons.state;
	old = tp->buttons.old_state;

	/* FIXME: clickpads have only one physical button. If we cater for
	   other devices, we need to handle this here */

	if ((current & 0x1) ^ (old  & 0x1)) {
		unsigned int button;
		if (current & 0x1) {
			button = guess_softbutton(tp, userdata);
			tp->buttons.active_softbutton = button;
		} else {
			button = tp->buttons.active_softbutton;
			tp->buttons.active_softbutton = 0;
		}
		tp->interface->button(tp, userdata, button, !!(current & 0x1));
	}

	tp->buttons.old_state = tp->buttons.state;
	return 0;
}

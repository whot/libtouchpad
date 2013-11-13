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

#include <touchpad-int.h>
#include <touchpad-config.h>

#include <assert.h>

struct tap_config tap_defaults = {
	.enabled = true,
	.timeout_period = 180,
	.move_threshold = 30,
};

struct scroll_config scroll_defaults = {
	.methods = TOUCHPAD_SCROLL_TWOFINGER_VERTICAL,
	.vdelta = 100,
	.hdelta = 100,
};

int
touchpad_config_tap_set_defaults(struct touchpad *tp)
{
	tp->tap.config = tap_defaults;

	return 0;
}


int
touchpad_config_tap_set(struct touchpad *tp, bool enable,
			int timeout, int doubletap_timeout,
			int move_threshold)
{
	assert(tp);

	tp->tap.config.enabled = enable;
	tp->tap.config.timeout_period = timeout;
	/* FIXME: doubletap timeout */
	tp->tap.config.move_threshold = move_threshold;

	return 0;
}

int
touchpad_config_tap_get(struct touchpad *tp, bool *enabled,
			int *timeout, int *doubletap_timeout,
			int *move_threshold)
{
	if (enabled)
		*enabled = tp->tap.config.enabled;
	if (timeout)
		*timeout = tp->tap.config.timeout_period;
	if (doubletap_timeout)	/* FIXME: */
		*doubletap_timeout = tp->tap.config.timeout_period;
	if (move_threshold)
		*move_threshold = tp->tap.config.move_threshold;

	return 0;
}

int
touchpad_config_scroll_set(struct touchpad *tp,
			   enum touchpad_scroll_methods methods,
			   int vdelta, int hdelta)
{
	tp->scroll.config.methods = methods;
	tp->scroll.config.vdelta = vdelta;
	tp->scroll.config.hdelta = hdelta;
	return 0;
}

int
touchpad_config_scroll_get(struct touchpad *tp,
			   enum touchpad_scroll_methods *methods,
			   int *vdelta, int *hdelta)
{
	if (methods)
		*methods = tp->scroll.config.methods;
	if (vdelta)
		*vdelta = tp->scroll.config.vdelta;
	if (hdelta)
		*hdelta = tp->scroll.config.hdelta;
	return 0;
}

int
touchpad_config_scroll_set_defaults(struct touchpad *tp)
{
	tp->scroll.config = scroll_defaults;

	return 0;
}

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

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include "touchpad.h"

static void
motion(struct touchpad *tp, void *userdata, int x, int y)
{
	printf("motion: %d/%d\n", x, y);
}

static void
button(struct touchpad *tp, void *userdata, unsigned int button, bool is_press)
{
	printf("button: %d %s\n", button, is_press ? "press" : "release");
}

static void
scroll(struct touchpad *tp, void *userdata, enum touchpad_scroll_direction dir, int units)
{
	printf("scroll: %s %d\n", dir == TOUCHPAD_SCROLL_HORIZONTAL ? "horizontal" : "vertical", units);
}


const struct touchpad_interface interface = {
	.motion = motion,
	.button = button,
	.scroll = scroll,
};


int usage(void) {
	printf("usage: %s /dev/input/event0\n", program_invocation_short_name);
	return 1;
}

int mainloop(struct touchpad *tp) {
	struct pollfd fds;

	fds.fd = touchpad_get_fd(tp);
	fds.events = POLLIN;

	while (poll(&fds, 1, -1)) {
		touchpad_handle_events(tp, NULL);
	}

	return 0;
}

int main (int argc, char **argv) {
	const char *path;
	struct touchpad *tp;

	if (argc < 2)
		return usage();

	path = argv[1];

	tp = touchpad_new_from_path(path);
	touchpad_set_interface(tp, &interface);

	mainloop(tp);

	return 0;
}

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

#include <libevdev/libevdev.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "touchpad.h"

struct tpdata {
	int timerfd;
};

static void
motion(struct touchpad *tp, void *userdata, int x, int y)
{
	printf("%4d/%-4d\n", x, y);
}

static void
button(struct touchpad *tp, void *userdata, unsigned int button, bool is_press)
{
	printf("%10s %s (%s)\n", "", libevdev_event_code_get_name(EV_KEY, button), is_press ? "press" : "release");
}

static void
tap(struct touchpad *tp, void *userdata, unsigned int fingers, bool is_press)
{
	printf("%30s tap %d (%s)\n", "", fingers, is_press ? "press" : "release");
}

static void
scroll(struct touchpad *tp, void *userdata, enum touchpad_scroll_direction dir, double units)
{
	printf("%50s %s scroll: %.2f\n", "", dir == TOUCHPAD_SCROLL_HORIZONTAL ? "horizontal" : "vertical", units);
}

static int
register_timer(struct touchpad *tp, void *userdata, unsigned int now, unsigned int ms)
{
	int rc;
	struct tpdata *tpdata = userdata;
	struct itimerspec t;

	t.it_value.tv_sec = ms/1000;
	t.it_value.tv_nsec = (ms % 1000) * 1000 * 1000;
	t.it_interval.tv_sec = 0;
	t.it_interval.tv_nsec = 0;

	rc = timerfd_settime(tpdata->timerfd, 0, &t, NULL);
	return rc < 0 ? -errno : 0;
}

const struct touchpad_interface interface = {
	.motion = motion,
	.button = button,
	.scroll = scroll,
	.tap = tap,
	.register_timer = register_timer,
};

int usage(void) {
	printf("usage: %s /dev/input/event0\n", program_invocation_short_name);
	return 1;
}

int mainloop(struct touchpad *tp, struct tpdata *data) {
	struct pollfd fds[2];

	fds[0].fd = touchpad_get_fd(tp);
	fds[0].events = POLLIN;
	fds[1].fd = data->timerfd;
	fds[1].events = POLLIN;

	while (poll(fds, 2, -1)) {
		struct timespec t;
		unsigned int millis;

		clock_gettime(CLOCK_REALTIME, &t);
		millis = t.tv_sec * 1000 + t.tv_nsec/1000000;

		touchpad_handle_events(tp, data, millis);

		if (fds[1].revents) {
			uint64_t buf;
			read(fds[1].fd, &buf, sizeof(buf));
		}
	}

	return 0;
}

int main (int argc, char **argv) {
	int rc;
	int fd;
	const char *path;
	struct touchpad *tp;
	struct tpdata tpdata;

	if (argc < 2)
		return usage();

	path = argv[1];
	fd = open(path, O_RDONLY|O_NONBLOCK);
	if (fd < 0) {
		fprintf(stderr, "Error opening the device: %s\n", strerror(errno));
		return 1;
	}

	if (!isatty(fileno(stdout)))
		setbuf(stdout, NULL);

	rc = touchpad_new_from_fd(fd, &tp);
	assert(rc == 0);
	touchpad_set_interface(tp, &interface);

	tpdata.timerfd = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC|TFD_NONBLOCK);
	assert(tpdata.timerfd > -1);

	mainloop(tp, &tpdata);

	close(fd);

	return 0;
}

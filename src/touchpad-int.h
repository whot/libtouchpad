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

#ifndef TOUCHPAD_INT_H
#define TOUCHPAD_INT_H

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>
#include "touchpad.h"
#include "touchpad-util.h"

#define MAX_TOUCHPOINTS 10 /* update when mutants are commonplace */
#define MAX_MOTION_HISTORY_SIZE 10
#define MAX_TAP_EVENTS 10

enum touch_state {
	TOUCH_NONE = 7,
	TOUCH_BEGIN,
	TOUCH_UPDATE,
	TOUCH_END,
};

struct touch_history_point {
	int x;
	int y;
	unsigned int millis;
};

struct touch_history {
	struct touch_history_point points[MAX_MOTION_HISTORY_SIZE];
	unsigned int index;
	size_t valid;
	size_t size;
};

struct touch {
	bool dirty;
	bool pointer; /* is this the pointer-moving touchpoint? */

	enum touch_state state;
	int millis;
	int x, y;

	unsigned int number;
	struct touch_history history;
};

enum tap_state {
	TAP_STATE_IDLE = 4,
	TAP_STATE_TOUCH,
	TAP_STATE_HOLD,
	TAP_STATE_TAPPED,
	TAP_STATE_TOUCH_2,
	TAP_STATE_TOUCH_2_HOLD,
	TAP_STATE_TOUCH_3,
	TAP_STATE_TOUCH_3_HOLD,
	TAP_STATE_DRAGGING_OR_DOUBLETAP,
	TAP_STATE_DRAGGING,
	TAP_STATE_DRAGGING_2,
	TAP_STATE_DEAD, /**< finger count exceeded */
};

enum tap_event {
	TAP_EVENT_TOUCH = 12,
	TAP_EVENT_MOTION,
	TAP_EVENT_RELEASE,
	TAP_EVENT_TIMEOUT,
};

struct tap_config {
	bool enabled;
	unsigned int timeout_period;
	unsigned int move_threshold;
};

struct tap {
	struct tap_config config;
	unsigned int timeout;
	enum tap_state state;
	enum tap_event events[MAX_TAP_EVENTS];
};

enum scroll_state {
	SCROLL_STATE_NONE = 9,
	SCROLL_STATE_SCROLLING,
};

struct scroll_config {
	enum touchpad_scroll_methods methods;
	int hdelta;
	int vdelta;
};

struct scroll {
	struct scroll_config config;
	enum scroll_state state;
	enum touchpad_scroll_direction direction;
};

struct buttons {
	uint32_t state;
	uint32_t old_state;
};

enum event_types {
	EVENT_NONE = 0,
	EVENT_BUTTON_PRESS = 0x1,
	EVENT_BUTTON_RELEASE = 0x2,
	EVENT_MOTION = 0x4,
};


struct touchpad {
    struct libevdev *dev;
    char *path;
    int fingers_down;		/* number of fingers down */
    int slot;			/* current slot */

    int ntouches;		/* from ABS_MT_SLOT(max) */
    struct touch touches[MAX_TOUCHPOINTS];

    struct buttons buttons;
    struct tap tap;
    struct scroll scroll;
    const struct touchpad_interface *interface;

    unsigned int ms;		/* ms of last SYN_REPORT */

    enum event_types queued;
};

static inline struct touch*
touchpad_pointer_touch(struct touchpad *tp)
{
	int i;
	for (i = 0; i < tp->ntouches; i++) {
		struct touch *t = &tp->touches[i];
		if (t->pointer)
			return t;
	}

	return NULL;
}

static inline struct touch*
touchpad_touch(struct touchpad *tp, int index)
{
	return &tp->touches[index];
}

static inline struct touch*
touchpad_current_touch(struct touchpad *tp)
{
	return (tp->slot != -1) ? touchpad_touch(tp, tp->slot) : NULL;
}

int touchpad_handle_event(struct touchpad *tp,
			  void *userdata,
			  const struct input_event *ev);

void touchpad_motion_dejitter(struct touch *t);
void touchpad_motion_to_delta(struct touch *t, int *dx, int *dy);
void touchpad_apply_motion_history(const struct touchpad *tp, struct touch *t);
void touchpad_history_reset(struct touch *t);
void touchpad_history_push(struct touch *t, int x, int y, unsigned int millis);
struct touch_history_point * touchpad_history_get(struct touch *t, int when);
struct touch_history_point * touchpad_history_get_last(struct touch *t);
int touchpad_tap_handle_state(struct touchpad *tp, void *userdata);
int touchpad_tap_handle_timeout(struct touchpad *tp, unsigned int ms, void *userdata);
int touchpad_scroll_handle_state(struct touchpad *tp, void *userdata);
void touchpad_log(const char *fmt, ...);

#endif

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

void touchpad_error_log(const char *msg, ...);
#define argcheck_log(_file, _line, _func, msg, ...)  \
	touchpad_error_log("%s:%d %s(): " msg, _file, _line, _func, ##__VA_ARGS__)
#include <ccan/argcheck/argcheck.h>


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

enum button_state {
	BUTTON_STATE_NONE = 16,
	BUTTON_STATE_AREA,
	BUTTON_STATE_LEFT,
	BUTTON_STATE_LEFT_NEW,
	BUTTON_STATE_RIGHT,
	BUTTON_STATE_RIGHT_NEW,
	BUTTON_STATE_LEFT_TO_AREA,
	BUTTON_STATE_RIGHT_TO_AREA,
	BUTTON_STATE_LEFT_TO_RIGHT,
	BUTTON_STATE_RIGHT_TO_LEFT,
	BUTTON_STATE_PRESSED_RIGHT,
	BUTTON_STATE_PRESSED_LEFT,
};

enum button_event {
	BUTTON_EVENT_IN_R = 30,
	BUTTON_EVENT_IN_L,
	BUTTON_EVENT_IN_AREA,
	BUTTON_EVENT_UP,
	BUTTON_EVENT_PRESS,
	BUTTON_EVENT_RELEASE,
	BUTTON_EVENT_TIMEOUT,
};


struct touch {
	bool dirty;
	bool pointer; /**< is this the pointer-moving touchpoint? */
	bool pinned; /**< touch is pinned from phys. button press, movement is ignored */
	bool fake; /**< touch is a fake touch from BTN_TOOL_*TAP */

	enum touch_state state;
	int millis;
	int x, y;

	unsigned int number;
	struct touch_history history;

	enum button_state button_state; /**< state for softbuttons */
	unsigned int button_timeout;
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
	TAP_STATE_DRAGGING_WAIT,
	TAP_STATE_DRAGGING_2,
	TAP_STATE_DEAD, /**< finger count exceeded */
};

enum tap_event {
	TAP_EVENT_TOUCH = 12,
	TAP_EVENT_MOTION,
	TAP_EVENT_RELEASE,
	TAP_EVENT_BUTTON,
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

struct button_config {
	int top, bottom;
	int right[2]; /* left, right */
	unsigned int leave_timeout;
	unsigned int enter_timeout;
};

struct buttons {
	struct button_config config;
	/* currently active soft-button, used for release event in case
	 * the area changes between press and release */
	int active_softbutton;

	uint32_t state;
	uint32_t old_state;

	unsigned int timeout;

	/**
	 * Process the current touchpad state, different for clickpads and
	 * traditional touchpads.
	 */
	int (*handle_state)(struct touchpad *tp, void *userdata);
	int (*handle_timeout)(struct touchpad *tp, unsigned int now, void *userdata);
	bool (*select_pointer_touch)(struct touchpad *tp, struct touch *t);
};

enum event_types {
	EVENT_NONE = 0,
	EVENT_BUTTON_PRESS = 0x1,
	EVENT_BUTTON_RELEASE = 0x2,
	EVENT_MOTION = 0x4,
};

struct touchpad_config {
	size_t motion_history_size;
};


struct touchpad {
    struct libevdev *dev;
    int fingers_down;		/* number of fingers down */
    int slot;			/* current slot */

    int maxtouches;		/* from ABS_MT_SLOT(max) */
    int ntouches;		/* maxtouches + triple/quad if applicable */
    struct touch touches[MAX_TOUCHPOINTS];

    struct touchpad_config config;
    struct buttons buttons;
    struct tap tap;
    struct scroll scroll;
    const struct touchpad_interface *interface;

    unsigned int ms;		/* ms of last SYN_REPORT */

    enum event_types queued;

    int timerfd;
    int epollfd;
    unsigned int next_timeout;

    struct {
	    touchpad_log_func_t func;
	    void *data;
    } log;
};

void
touchpad_log(struct touchpad *tp,
	     enum touchpad_log_priority priority,
	     const char *format, ...);

#define touchpad_for_each_touch(_tp, _t) \
	for (int _i = 0; (_t = touchpad_touch(_tp, _i)) && _i < tp->ntouches; _i++)

static inline struct touch*
touchpad_touch(struct touchpad *tp, int index)
{
	return &tp->touches[index];
}

static inline struct touch*
touchpad_pointer_touch(struct touchpad *tp)
{
	struct touch *t;
	touchpad_for_each_touch(tp, t) {
		if (t->pointer) {
			argcheck_int_ne(t->state, TOUCH_NONE);
			return t;
		}
	}

	return NULL;
}

static inline struct touch*
touchpad_pinned_touch(struct touchpad *tp)
{
	struct touch *t;
	touchpad_for_each_touch(tp, t)
		if (t->pinned)
			return t;

	return NULL;
}

static inline struct touch*
touchpad_current_touch(struct touchpad *tp)
{
	return (tp->slot != -1 && tp->slot < MAX_TOUCHPOINTS) ? touchpad_touch(tp, tp->slot) : NULL;
}

static inline struct touch*
touchpad_fake_touch(struct touchpad *tp, int which)
{
	argcheck_int_range(which, BTN_TOOL_DOUBLETAP, BTN_TOOL_QUADTAP);

	return touchpad_touch(tp, tp->maxtouches + which - BTN_TOOL_DOUBLETAP);
}

int touchpad_handle_event(struct touchpad *tp,
			  void *userdata,
			  const struct input_event *ev);

void touchpad_motion_dejitter(struct touch *t);
void touchpad_motion_to_delta(struct touch *t, int *dx, int *dy);
void touchpad_apply_motion_history(const struct touchpad *tp, struct touch *t);
void touchpad_history_reset(struct touchpad *tp, struct touch *t);
void touchpad_history_push(struct touch *t, int x, int y, unsigned int millis);
struct touch_history_point * touchpad_history_get(struct touch *t, int when);
struct touch_history_point * touchpad_history_get_last(struct touch *t);
int touchpad_tap_handle_state(struct touchpad *tp, void *userdata);
unsigned int touchpad_tap_handle_timeout(struct touchpad *tp, unsigned int ms, void *userdata);
int touchpad_scroll_handle_state(struct touchpad *tp, void *userdata);
int touchpad_button_handle_state(struct touchpad *tp, void *userdata);
bool touchpad_button_select_pointer_touch(struct touchpad *tp, struct touch *t);
int touchpad_button_handle_timeout(struct touchpad *tp, unsigned int now, void *userdata);
int touchpad_phys_button_handle_state(struct touchpad *tp, void *userdata);
bool touchpad_phys_button_select_pointer_touch(struct touchpad *tp, struct touch *t);
int touchpad_phys_button_handle_timeout(struct touchpad *tp, unsigned int now, void *userdata);
int touchpad_request_timer(struct touchpad *tp, void *userdata, unsigned int now, unsigned int delta);

void touchpad_config_set_dynamic_defaults(struct touchpad *tp);
void touchpad_config_set_static_defaults(struct touchpad *tp);
#endif

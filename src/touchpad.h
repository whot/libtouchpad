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

#ifndef TOUCHPAD_H
#define TOUCHPAD_H

#include <stdbool.h>
#include <stdarg.h>

/**
 * @mainpage
 *
 * **libtouchpad** is a library for handling touchpad events, including the
 * conversion of events to motion, scroll, button and other events. It
 * abstracts the actual touchpad handling to be usable from various
 * backends. These backends include a simple commandline tool, an X.Org
 * input driver and (in the future) drivers for wayland compositors.
 *
 * Requirements
 * ============
 * **libtouchpad** requires [libevdev](http://www.freedesktop.org/software/libevdev/)
 *
 *
 * License information
 * ===================
 * libevdev is licensed under the
 * [X11 license](http://cgit.freedesktop.org/libevdev/tree/COPYING).
 */

/**
 * @page tapping Tap-to-Click behavior and configuration
 *
 * Tap-to-click is a behavior where a short finger press and release
 * triggers a button press and release. For a touch to be converted to such
 * a button press the following conditions must be met:
 * + the release of the finger is within the tap timeout
 * + the finger must move less than the tap move threshold
 *
 * Two taps in quick succession (within the permitted timeout) will cause
 * two button events to be generated.
 *
 * Note that libtouchpad differs between tapping and button events see (@ref
 * callbackinterface). In the context of tapping, a "button event" is the
 * event generated by a tap and can be either a press or release event.
 * Most use-cases will handle a single-finger tap event like a left mouse button
 * event.
 *
 * Multi-finger tapping
 * ====================
 *
 * A multi-finger tap is defined as one or more fingers down, with the last
 * finger defining the actual finger number. i.e. one finger down and a
 * second finger tapping will trigger a two-finger tap, provided the above
 * conditions are met for the last finger.
 *
 * libtouchpad currently supports up to three-finger taps.
 *
 * Tap-and-Drag
 * ============
 *
 * If a tap is triggered and followed by a touch-and-hold, the button is
 * only pressed once and held down. The button is released once the last
 * finger leaves the touchpad, allowing a user to e.g. drag an item.
 *
 * Documentation
 * =============
 * See [tap state diagram](touchpad-tap-state-machine.svg) for a graph.
 */

/**
 * @page scrolling Scrolling gesture recognition
 *
 * libtouchpad supports two scroll methods: two-finger scrolling and
 * (eventually) edge scrolling
 *
 * Two-finger Scrolling
 * ====================
 * In two-finger scrolling, a movement by more than a given threshold in a
 * direction will trigger a scroll event. For two-finger scrolling to work,
 * exactly two fingers must be on the touchpad. Moving both fingers or
 * holding one and moving the second will trigger scroll events.
 *
 * Two-finger scrolling terminates when one finger leaves the touchpad or a
 * third finger is placed onto the touchpad.
 *
 * Edge Scrolling
 * ====================
 * **Note: not implemented**
 *
 * In edge scrolling, a movement of exactly one fingers along a defined edge
 * by more than a given threshold will trigger a scroll event.
 *
 * Edge scrolling terminates when the finger leaves the touchpad or a second
 * finger is placed onto the touchpad.
 *
 * Scroll Direction Lock
 * =====================
 * Once a scroll gesture has started, the scroll direction and method is
 * locked and will not change until the scroll terminates. For example, if a
 * vertical two-finger scroll has been triggerd, sideways movement will not
 * trigger horizontal scrolling.
 */

/**
 * @page softbuttons Software-button emulation
 *
 * Clickpads provide only one physical button, BTN_LEFT. Right-button
 * clicks are emulated based on the location of the finger at click-time.
 *
 * libtouchpad supports software buttons, for left, right and middle clicks.
 * The buttons are always aligned in a horizontal layout of user-defined
 * height. Note that a button click with the finger outside of the button
 * area will always result in a left click.
 *
 *      +------------------------+   +------------------------+   +------------------------+
 *	|                        |   |    LEFT    |   RIGHT   |   |                        |
 *	|                        |   +------------------------+   |                        |
 *	|          LEFT          |   |                        |   |          LEFT          |
 *	|                        |   |                        |   |                        |
 *	|                        |   |                        |   |                        |
 *	+------------------------+   |                        |   +------------------------+
 *	|    LEFT    |   RIGHT   |   |                        |   |          RIGHT         |
 *	+------------------------+   +------------------------+   +------------------------+
 *
 * Button selection
 * ================
 * A button click for a right button is generated if
 * * the finger clicking is on top of the right button area, or
 * * any other finger is on top of the right button area when a click happens
 *
 * A button click for a right button is not generated if
 * * the finger started outside of the button area and moved into the button
 *   area before the click
 *
 * Finger behavior
 * ===============
 * A finger inside the button area does not generate movement. For the left
 * button area this only affects the area shared with the right button area,
 * not the rest of the touchpad.
 *
 * If a finger leaves the button area and stays outside past a timeout, the
 * finger may generate movement without the need for releasing the finger.
 */

/**
 * @defgroup callbackinterface Callback interface
 *
 * Event notifications are sent to the callers through a callback interface.
 */

/**
 * @defgroup api libtouchpad API
 *
 * The API to initialize a touchpad device and handle events.
 */

struct touchpad;

/**
 * Input parameter into the struct touchpad_interface::scroll
 * callback.
 */
enum touchpad_scroll_direction {
	TOUCHPAD_SCROLL_HORIZONTAL = 3,
	TOUCHPAD_SCROLL_VERTICAL,
};

enum touchpad_scroll_methods {
	TOUCHPAD_SCROLL_NONE = 0x0,
	TOUCHPAD_SCROLL_EDGE_VERTICAL = 0x1,
	TOUCHPAD_SCROLL_EDGE_HORIZONTAL = 0x2,
	TOUCHPAD_SCROLL_TWOFINGER_VERTICAL = 0x4,
	TOUCHPAD_SCROLL_TWOFINGER_HORIZONTAL = 0x8,
};

/**
 * @ingroup callbackinterface
 *
 * Callback interface used by the backends to get notified about
 * events on the touchpad.
 */
struct touchpad_interface {
	/**
	 * Called for relative motion event compared to the previous position.
	 *
	 * @param tp The touchpad device
	 * @param userdata Backend-specific data, see
	 *	  touchpad_handle_events()
	 * @param x Unaccelerated but de-jittered x coordinate delta
	 * @param y Unaccelerated but de-jittered y coordinate delta
	 */
	void (*motion)(struct touchpad *tp, void *userdata, int x, int y);
	/**
	 * Called for a physical button event
	 *
	 * @param tp The touchpad device
	 * @param userdata Backend-specific data, see
	 *	  touchpad_handle_events()
	 * @param button The button number pressed, see linux/input.h for a
	 *	  range of possible buttons.
	 * @param is_press True for a press event, false for a release event
	 */
	void (*button)(struct touchpad *tp,
		      void *userdata,
		      unsigned int button,
		      bool is_press);

	/**
	 * Called for a tap event. If tapping is enabled and a tap
	 * registers, this callback is invoked with the number of fingers
	 * triggering this tap event.
	 *
	 * @param tp The touchpad device
	 * @param userdata Backend-specific data, see
	 *	  touchpad_handle_events()
	 * @param fingers The number of fingers causing the tap event
	 * @param is_press True for a press event, false for a release event
	 */
	void (*tap)(struct touchpad *tp,
		    void *userdata,
		    unsigned int fingers,
		    bool is_press);
	/**
	 * Called for a scroll event. The first scroll event will always be
	 * 1 unit or more, other scroll events may be less than one unit. A
	 * unit count of 0 signals that scrolling has terminated.
	 *
	 * @param tp The touchpad device
	 * @param userdata Backend-specific data, see
	 *	  touchpad_handle_events()
	 * @param direction The scrolling direction
	 * @param units The number of units scrolled in that direction, or 0
	 *		if the scroll gesture terminated.
	 */
	void (*scroll)(struct touchpad *tp, void *userdata,
		       enum touchpad_scroll_direction direction, double units);

	/**
	 * Called for a rotate event, i.e. two fingers down rotating around
	 * each other.
	 *
	 * @param tp The touchpad device
	 * @param userdata Backend-specific data, see
	 *	  touchpad_handle_events()
	 * @param degrees The delta angle in degrees clockwise
	 */
	void (*rotate)(struct touchpad *tp, void *userdata, int degrees);

	/**
	 * Called for a pinch event, i.e. two fingers moving towards each
	 * other.
	 *
	 * @param tp The touchpad device
	 * @param userdata Backend-specific data, see
	 *	  touchpad_handle_events()
	 * @param scale The relative scale of the movement in percent to the
	 * previous position, i.e. 200 means the distance doubled.
	 */
	void (*pinch)(struct touchpad *tp, void *userdata, int scale);

	/**
	 * Called by libtouchpad to register a timer in ms millisecond from
	 * now. The backend must call touchpad_handle_timer_expired()
	 * after this period expires.
	 *
	 * The time used by libtouchpad is the time provided by the input
	 * events on the device fd. By default, this is CLOCK_REALTIME, but
	 * it may be set to CLOCK_MONOTONIC if the kernel supports the
	 * EVIOCSCLOCK ioctl.
	 *
	 * @param tp The touchpad device
	 * @param userdata Backend-specific data, see touchpad_handle_events()
	 * @param now The current time in milliseconds.
	 * @param ms The timer expiry time in milliseconds, relative to now.
	 * If the timer expiry time is 0, the backend should unregister any
	 * current timers.
	 *
	 * @return 0 on success or a negative errno on failure
	 */
	int (*register_timer)(struct touchpad *tp, void *userdata, unsigned int now, unsigned int ms);
};

/**
 * @ingroup api
 *
 * Create a new touchpad device from the given path. The caller must
 * check permissions.
 *
 * @param path The path to the device file
 * @param tp Set to the new touchpad device, undefined on failure
 * @return 0 on success or a negative errno on failure.
 */
int touchpad_new_from_path(const char *path, struct touchpad **tp);
/**
 * @ingroup api
 * Free the touchpad device.
 */
void touchpad_free(struct touchpad *tp);
/**
 * @ingroup api
 *
 * Set the file descriptor for an existing touchpad device. If the device
 * was opened externally and the fd changed, use this call to change the
 * internal fd.
 * @param tp A previously opened touchpad device
 * @param fd The new file descriptor
 *
 * @return 0 on success or a negative errno on failure
 */
int touchpad_change_fd(struct touchpad *tp, int fd);
/**
 * @ingroup api
 * Re-open the path used in touchpad_new_from_path()
 *
 * @param tp A previously opened touchpad device
 * @return 0 on success or a negative errno on failure
 */
int touchpad_reopen(struct touchpad *tp);
/**
 * @ingroup api
 *
 * Close the current fd for this device but don't free any information.
 *
 * @param tp A previously opened touchpad device
 * @return 0 on success or a negative errno on failure
 */
int touchpad_close(struct touchpad *tp);
/**
 * @ingroup api
 *
 * Get axis information from the device.
 * @param tp A previously opened touchpad device
 * @param axis An absolute axis code as defined in linux/input.h (ABS_X, ABS_Y..)
 * @param min If not NULL, min is set to the minimum value for this axis
 * @param max If not NULL, max is set to the maximum value for this axis
 * @param res If not NULL, res is set to the resolution of this axis
 * @return 0 on success or -1 if the axis is not available on this device
 */
int touchpad_get_min_max(struct touchpad *tp, int axis, int *min, int *max, int *res);
/**
 * @ingroup api
 *
 * Set the interface for event handling
 * @param tp A previously opened touchpad device
 * @param interface The callback interface
 */
void touchpad_set_interface(struct touchpad *tp, const struct touchpad_interface *interface);
/**
 * @ingroup api
 *
 * Read and handle events from this device
 * @param tp A previously opened touchpad device
 * @param userdata The data to be supplied in the callback interface.
 * @param now The current time in milliseconds
 *
 * @return 0 on success or a negative errno on failure
 */
int touchpad_handle_events(struct touchpad *tp, void *userdata, unsigned int now);

/**
 * @ingroup api
 *
 * @return The current file descriptor in use
 */
int touchpad_get_fd(struct touchpad *tp);

/**
 * @return the backend libevdev device
 */
struct libevdev *touchpad_get_device(struct touchpad *tp);

enum touchpad_log_priority {
	TOUCHPAD_LOG_ERROR = 10,        /**< critical errors and application bugs */
	TOUCHPAD_LOG_INFO  = 20,        /**< informational messages */
	TOUCHPAD_LOG_BUG = 30,          /**< internal bugs */
	TOUCHPAD_LOG_DEBUG = 40         /**< debug information */
};

/**
 * Logging function called by library-internal logging.
 * This function is expected to treat its input like printf would.
 *
 * @param tp A previously opened touchpad device
 * @param priority Log priority of this message
 * @param data User-supplied data pointer (see libevdev_set_log_function())
 * @param format printf-style format string
 * @param args List of arguments
 */

typedef void (*touchpad_log_func_t)(struct touchpad *tp,
				    enum touchpad_log_priority priority,
				    void *data,
				    const char *format, va_list args);
/**
 * Set the logging function for this device. This function is called for any
 * output logging from inside the touchpad. The default is printf.
 *
 * @param tp A previously opened touchpad device
 * @param logfunc Log function to call
 * @param userdata The data to be supplied in the callback interface.
 */
void touchpad_set_log_func(struct touchpad *tp, touchpad_log_func_t logfunc, void *userdata);

typedef void (*touchpad_error_log_func_t)(const char *format, va_list args);
/**
 * Set the library-wide logging function for internal errors. This function
 * is called when something goes wrong. Look at the output of this, it
 * always indicates a bug. Default is fprintf(stderr).
 */
void touchpad_set_error_log_func(touchpad_error_log_func_t func);

#endif

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


#ifndef TOUCHPAD_CONFIG_H
#define TOUCHPAD_CONFIG_H

#include <limits.h>
#include <touchpad.h>

/**
 * @defgroup configuration Parameter configuration
 *
 * This section describes the parameter changes to adjust behavior and
 * functionality.
 */

enum touchpad_config_parameter {
	TOUCHPAD_CONFIG_NONE = 0,
	TOUCHPAD_CONFIG_TAP_ENABLE,
	TOUCHPAD_CONFIG_TAP_TIMEOUT,
	TOUCHPAD_CONFIG_TAP_DOUBLETAP_TIMEOUT,
	TOUCHPAD_CONFIG_TAP_MOVE_THRESHOLD,
	TOUCHPAD_CONFIG_SCROLL_METHOD,
	TOUCHPAD_CONFIG_SCROLL_DELTA_VERT,
	TOUCHPAD_CONFIG_SCROLL_DELTA_HORIZ,
	TOUCHPAD_CONFIG_MOTION_HISTORY_SIZE,
	TOUCHPAD_CONFIG_SOFTBUTTON_TOP, /* in % of the height */
	TOUCHPAD_CONFIG_SOFTBUTTON_BOTTOM, /* 0 for bottom, 1 for top */
	TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_RIGHT, /* in % of the touchpad width */
	TOUCHPAD_CONFIG_SOFTBUTTON_RBTN_LEFT, /* in % of the touchpad width*/
	/**
	 * The timeout in ms until leaving a software-defined button counts
	 * as leaving a button proper. For example, if the finger moves from
	 * the right-button area into the general touchpad area, after this
	 * timeout the finger will not generate right-button events anymore.
	 */
	TOUCHPAD_CONFIG_SOFTBUTTON_LEAVE_TIMEOUT,

	/**
	 * The timeout in ms until entering a software-defined button counts
	 * as entering that button proper. For example, if the finger starts
	 * in the left button area but moves to the right button area within
	 * this timeout (and stays there until the timeout expires), the
	 * finger will trigger right button events.
	 */
	TOUCHPAD_CONFIG_SOFTBUTTON_ENTER_TIMEOUT,

	TOUCHPAD_CONFIG_LAST,
	/**
	 * Use the built-in defaults for the preceding parameter.
	 */
	TOUCHPAD_CONFIG_USE_DEFAULT = INT_MAX,
};

enum touchpad_config_error {
	TOUCHPAD_CONFIG_ERROR_NO_ERROR = 0,
	TOUCHPAD_CONFIG_ERROR_KEY_INVALID,
	TOUCHPAD_CONFIG_ERROR_VALUE_INVALID,
	TOUCHPAD_CONFIG_ERROR_VALUE_TOO_HIGH,
	TOUCHPAD_CONFIG_ERROR_VALUE_TOO_LOW,
	TOUCHPAD_CONFIG_ERROR_NOT_SUPPORTED, /**< The HW cannot enable this configuration */
};

/**
 * Change the touchpad configuration parameters. The vararg is a key/value
 * pair of touchpad_config_parameter as key and the value to be set as
 * value, terminated by a single TOUCHPAD_CONFIG_NONE key.
 *
 * For example, to enable tapping with a timeout of 100ms, use
 *
 * @code
 * rc = touchpad_config_set(tp,
 *	                    TOUCHPAD_CONFIG_TAP_ENABLE, 1,
 *			    TOUCHPAD_CONFIG_TAP_TIMEOUT, 100,
 *			    TOUCHPAD_CONFIG_NONE);
 * assert(rc == 0); // expect both to be successful
 * @endcode
 *
 * The special value TOUCHPAD_CONFIG_USE_DEFAULT may be used to revert to
 * the built-in defaults for the given key.
 *
 * If a key or value is invalid, the return value is the number of that
 * key/value pair, starting at 1. If the key is invalid, error is set to
 * TOUCHPAD_CONFIG_ERROR_KEY_INVALID. Otherwise, the value does not meet the
 * ranges required and error indicates the reason.
 * On error, values up to excluding the index returned are applied to the
 * touchpad. Processing stops at the first invalid index.
 *
 * @param tp A previously opened touchpad device
 * @param error If not NULL, set to the error code of the failed
 * keyconfiguration
 * @param ... A key/value pair of parameters and values
 * @return 0 on success, or the 1-indexed key or value that failed.
 */
int touchpad_config_set(struct touchpad *tp, enum touchpad_config_error *error, ...);

/**
 * Retrieve the touchpad configuration parameters. The vararg is a key/value
 * pair of touchpad_config_parameter as key and the value to be retrieved as
 * value, terminated by a single TOUCHPAD_CONFIG_NONE key.
 *
 * For example, to get the tapping parameters, use
 *
 * @code
 * int enabled, timeout;
 * rc = touchpad_config_set(tp,
 *	                    TOUCHPAD_CONFIG_TAP_ENABLE, &enabled,
 *			    TOUCHPAD_CONFIG_TAP_TIMEOUT, &timeout,
 *			    TOUCHPAD_CONFIG_NONE);
 * assert(rc == 0); // expect both to be successful
 * @endcode
 *
 * A NULL value as value is invalid and will lead to dead kittens.
 *
 * If a key is invalid, the return value is the number of that
 * key/value pair, starting at 1.
 * If a value is invalid, the return value is the negative number of the
 * key/value pair, starting at 1.
 * On error, values up to excluding the index returned are filled in, and
 * the value for all others is undefined.
 *
 * @param tp A previously opened touchpad device
 * @param ... A key/value pair of parameters and pointers to values
 * @return 0 on success, or the 1-indexed key or value that failed.
 */
int touchpad_config_get(struct touchpad *tp, ...);

/**
 * Restore built-in defaults. Note that some of these defaults are
 * hardcoded, others may be calculated based on hardware capabilities.
 * @param tp A previously opened touchpad
 */
void touchpad_config_set_defaults(struct touchpad *tp);
#endif

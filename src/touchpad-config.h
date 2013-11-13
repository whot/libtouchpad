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

#include <touchpad.h>

/**
 * @defgroup configuration Parameter configuration
 *
 * This section describes the parameter changes to adjust behavior and
 * functionality.
 */

/**
 * @ingroup configuration
 *
 * Modify tapping-related parameters. See @ref tapping for a detailed
 * description.
 *
 * @param tp A previously opened touchpad device
 * @param enable True to enable tapping, false to disable tapping
 * @param timeout Maximum time in ms to elapse after a touch-and-hold for a
 * a release to trigger a tap event.
 * @param doubletap_timeout Maximum time in ms to elapse between two taps to
 * count as doubletap.
 * @param move_threshold Maximum movement in device coordinates before a
 * release won't trigger a tap.
 *
 * @return 0 on success, -1 on failure
 */
int touchpad_config_tap_set(struct touchpad *tp, bool enable,
			    int timeout, int doubletap_timeout,
			    int move_threshold);
/**
 * @ingroup configuration
 *
 * Get tapping-related parameters. See @ref tapping for a detailed
 * description.
 *
 * @param tp A previously opened touchpad device
 * @param enabled If not NULL, set to true if tapping is enabled or false if
 * tapping is disabled.
 * @param timeout If not NULL, set to the tapping timeout
 * @param doubletap_timeout If not NULL, set to the doubletap timeout
 * @param move_threshold If not NULL, set to the move threshold
 *
 * @return 0 on success, -1 on failure
 */
int touchpad_config_tap_get(struct touchpad *tp, bool *enabled,
			    int *timeout, int *doubletap_timeout,
			    int *move_threshold);
/**
 * @ingroup configuration
 * Set the touchpad's tapping configuration to the built-in defaults.
 * @param tp A previously opened touchpad device
 *
 * @return 0 on success, -1 on failure
 */
int touchpad_config_tap_set_defaults(struct touchpad *tp);

/**
 * @ingroup configuration
 *
 * Modify scrolling-related parameters. See @ref scrolling for a detailed
 * description.
 *
 * @param tp A previously opened touchpad device
 * @param methods A bitmask of scrolling methods to enable
 * @param vdelta Delta movement in device coordinates that represents one
 * unit scrolling vertically
 * @param hdelta Delta movement in device coordinates that represents one
 * unit of scrolling horizontally
 *
 * @return 0 on success, -1 on failure
 */
int touchpad_config_scroll_set(struct touchpad *tp,
			       enum touchpad_scroll_methods methods,
			       int vdelta, int hdelta);
/**
 * @ingroup configuration
 *
 * Get scrolling-related parameters. See @ref scrolling for a detailed
 * description.
 *
 * @param tp A previously opened touchpad device
 * @param methods If not NULL, set to the bitmask of scrolling methods
 * currently enabled
 * @param vdelta If not NULL, set to the delta movement in device
 * coordinates that represents one unit scrolling vertically
 * @param hdelta if not NULL, set to the delta movement in device
 * coordinates that represents one unit of scrolling horizontally
 
 * @return 0 on success, -1 on failure
 */
int touchpad_config_scroll_get(struct touchpad *tp,
			       enum touchpad_scroll_methods *methods,
			       int *vdelta, int *hdelta);
int touchpad_config_scroll_set_defaults(struct touchpad *tp);
#endif

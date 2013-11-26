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
#include <stdarg.h>

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

struct touchpad_config touchpad_defaults = {
	.motion_history_size = 10,
};

struct button_config button_defaults_static = {
	.right = {0, 0, 0, 0},
	.middle = {0, 0, 0, 0},
};

struct button_config button_defaults_dynamic = {
	.right = {50, 100, 82, 100},
	.middle = {0, 0, 0, 0},
};

#define apply_value(what, value, deflt) \
	if (value == TOUCHPAD_CONFIG_USE_DEFAULT) what = deflt; \
	else what = value;


static int
config_error(enum touchpad_config_error error, enum touchpad_config_error *error_out)
{
	arg_require_int_range(error, TOUCHPAD_CONFIG_ERROR_NO_ERROR, TOUCHPAD_CONFIG_ERROR_NOT_SUPPORTED);
	if (error_out)
		*error_out = error;

	return error != TOUCHPAD_CONFIG_ERROR_NO_ERROR;
}

static int
config_set_softbutton(struct touchpad *tp,
		      enum touchpad_config_error *error,
		      enum touchpad_config_parameter button,
		      int value)
{
	int idx;
	int min, max;
	int v;

	if (value < 0)
		return config_error(TOUCHPAD_CONFIG_ERROR_VALUE_TOO_LOW, error);
	if (value > 100)
		return config_error(TOUCHPAD_CONFIG_ERROR_VALUE_TOO_HIGH, error);

	switch (button) {
		case TOUCHPAD_CONFIG_SOFTBUTTON_RIGHT_EDGE_LEFT:
			idx = 0;
			touchpad_get_min_max(tp, ABS_MT_POSITION_X, &min, &max, NULL);
			break;
		case TOUCHPAD_CONFIG_SOFTBUTTON_RIGHT_EDGE_RIGHT:
			idx = 1;
			touchpad_get_min_max(tp, ABS_MT_POSITION_X, &min, &max, NULL);
			break;
		case TOUCHPAD_CONFIG_SOFTBUTTON_RIGHT_EDGE_TOP:
			idx = 2;
			touchpad_get_min_max(tp, ABS_MT_POSITION_Y, &min, &max, NULL);
			break;
		case TOUCHPAD_CONFIG_SOFTBUTTON_RIGHT_EDGE_BOTTOM:
			idx = 3;
			touchpad_get_min_max(tp, ABS_MT_POSITION_Y, &min, &max, NULL);
			break;
		default:
			log_bug(button, "invalid config param for softbuttons\n");
			return config_error(TOUCHPAD_CONFIG_ERROR_NOT_SUPPORTED, error);
	}

	/* scale to device dimensions */
	apply_value(value, value, button_defaults_dynamic.right[idx]);

	/* touchpads are notorious liars. if 100/0 is requested, use
	 * INT_MAX/INT_MIN to make sure we really catch any events. For all
	 * other values, use the min/max range announced, even if it's wrong
	 */
	if (value == 100)
		v = INT_MAX;
	else if (value == 0)
		v = INT_MIN;
	else
		v = 1.0 * (max - min + 1) * value/100.0 + min;

	tp->buttons.config.right[idx] = v;
	return config_error(TOUCHPAD_CONFIG_ERROR_NO_ERROR, error);
}

static int
config_get_softbutton(struct touchpad *tp,
		      enum touchpad_config_parameter button,
		      int *value)
{
	int idx;
	int min, max;
	int v;

	switch (button) {
		case TOUCHPAD_CONFIG_SOFTBUTTON_RIGHT_EDGE_LEFT:
			idx = 0;
			touchpad_get_min_max(tp, ABS_MT_POSITION_X, &min, &max, NULL);
			break;
		case TOUCHPAD_CONFIG_SOFTBUTTON_RIGHT_EDGE_RIGHT:
			idx = 1;
			touchpad_get_min_max(tp, ABS_MT_POSITION_X, &min, &max, NULL);
			break;
		case TOUCHPAD_CONFIG_SOFTBUTTON_RIGHT_EDGE_TOP:
			idx = 2;
			touchpad_get_min_max(tp, ABS_MT_POSITION_Y, &min, &max, NULL);
			break;
		case TOUCHPAD_CONFIG_SOFTBUTTON_RIGHT_EDGE_BOTTOM:
			idx = 3;
			touchpad_get_min_max(tp, ABS_MT_POSITION_Y, &min, &max, NULL);
			break;
		default:
			return 1;
	}

	v = tp->buttons.config.right[idx];

	if (v == INT_MAX)
		v = 100;
	else if (v == INT_MIN)
		v = 0;
	else
		v = (v - min) * 100.0/(max - min + 1) + 0.5;

	*value = v;

	return 0;
}

/**
 * @return 0 on success, 1 for a bad key, -1 for a bad value
 */
static int
touchpad_config_set_key_value(struct touchpad *tp,
			      enum touchpad_config_error *error,
			      enum touchpad_config_parameter key,
			      int value)
{
	arg_require_int_range(key, TOUCHPAD_CONFIG_TAP_ENABLE, TOUCHPAD_CONFIG_LAST);

	switch(key) {
		case TOUCHPAD_CONFIG_NONE: /* filtered before */
		case TOUCHPAD_CONFIG_USE_DEFAULT:
			return 1;

		case TOUCHPAD_CONFIG_TAP_ENABLE:
			apply_value(tp->tap.config.enabled, value, tap_defaults.enabled);
			break;
		case TOUCHPAD_CONFIG_TAP_TIMEOUT:
			apply_value(tp->tap.config.timeout_period, value, tap_defaults.timeout_period);
			break;
		case TOUCHPAD_CONFIG_TAP_DOUBLETAP_TIMEOUT:
			apply_value(tp->tap.config.timeout_period, value, tap_defaults.timeout_period);
			break;
		case TOUCHPAD_CONFIG_TAP_MOVE_THRESHOLD:
			apply_value(tp->tap.config.timeout_period, value, tap_defaults.timeout_period);
			break;
		case TOUCHPAD_CONFIG_SCROLL_METHOD:
			apply_value(tp->scroll.config.methods, value, scroll_defaults.methods);
			break;
		case TOUCHPAD_CONFIG_SCROLL_DELTA_VERT:
			apply_value(tp->scroll.config.vdelta, value, scroll_defaults.vdelta);
			break;
		case TOUCHPAD_CONFIG_SCROLL_DELTA_HORIZ:
			apply_value(tp->scroll.config.hdelta, value, scroll_defaults.hdelta);
			break;
		case TOUCHPAD_CONFIG_MOTION_HISTORY_SIZE:
			if (value <= 0)
				return config_error(TOUCHPAD_CONFIG_ERROR_VALUE_TOO_LOW, error);
			if (value >= MAX_MOTION_HISTORY_SIZE)
				return config_error(TOUCHPAD_CONFIG_ERROR_VALUE_TOO_HIGH, error);
			apply_value(tp->config.motion_history_size, value, touchpad_defaults.motion_history_size);
			break;
		case TOUCHPAD_CONFIG_SOFTBUTTON_RIGHT_EDGE_LEFT:
		case TOUCHPAD_CONFIG_SOFTBUTTON_RIGHT_EDGE_RIGHT:
		case TOUCHPAD_CONFIG_SOFTBUTTON_RIGHT_EDGE_TOP:
		case TOUCHPAD_CONFIG_SOFTBUTTON_RIGHT_EDGE_BOTTOM:
			return config_set_softbutton(tp, error, key, value);
		default:
			return config_error(TOUCHPAD_CONFIG_ERROR_KEY_INVALID, error);
	}

	return config_error(TOUCHPAD_CONFIG_ERROR_NO_ERROR, error);
}

int
touchpad_config_set(struct touchpad *tp, enum touchpad_config_error *error, ...)
{
	va_list args;
	int processed = 1;
	unsigned int key;
	int value;

	va_start(args, error);

	key = va_arg(args, unsigned int);
	while (key != TOUCHPAD_CONFIG_NONE) {
		int success;
		value = va_arg(args, int);
		success = touchpad_config_set_key_value(tp, error, key, value);
		if (success != 0)
			break;
		processed++;
		key = va_arg(args, unsigned int);
	}

	va_end(args);

	if (key == TOUCHPAD_CONFIG_NONE)
		processed = 0;
	return processed;
}

/**
 * @return 0 on success, 1 for a bad key, -1 for a bad value
 */
static int
touchpad_config_get_key_value(struct touchpad *tp,
			      enum touchpad_config_parameter key,
			      int *value)
{
	arg_require_int_range(key, TOUCHPAD_CONFIG_TAP_ENABLE, TOUCHPAD_CONFIG_LAST);

	if (value == NULL)
		return -1;

	switch(key) {
		case TOUCHPAD_CONFIG_NONE: /* filtered before */
		case TOUCHPAD_CONFIG_USE_DEFAULT:
			return 1;

		case TOUCHPAD_CONFIG_TAP_ENABLE:
			*value = tp->tap.config.enabled;
			break;
		case TOUCHPAD_CONFIG_TAP_TIMEOUT:
			*value = tp->tap.config.timeout_period;
			break;
		case TOUCHPAD_CONFIG_TAP_DOUBLETAP_TIMEOUT:
			*value = tp->tap.config.timeout_period;
			break;
		case TOUCHPAD_CONFIG_TAP_MOVE_THRESHOLD:
			*value = tp->tap.config.timeout_period;
			break;
		case TOUCHPAD_CONFIG_SCROLL_METHOD:
			*value = tp->scroll.config.methods;
			break;
		case TOUCHPAD_CONFIG_SCROLL_DELTA_VERT:
			*value = tp->scroll.config.vdelta;
			break;
		case TOUCHPAD_CONFIG_SCROLL_DELTA_HORIZ:
			*value = tp->scroll.config.hdelta;
			break;
		case TOUCHPAD_CONFIG_MOTION_HISTORY_SIZE:
			*value = tp->config.motion_history_size;
			break;
		case TOUCHPAD_CONFIG_SOFTBUTTON_RIGHT_EDGE_LEFT:
		case TOUCHPAD_CONFIG_SOFTBUTTON_RIGHT_EDGE_RIGHT:
		case TOUCHPAD_CONFIG_SOFTBUTTON_RIGHT_EDGE_TOP:
		case TOUCHPAD_CONFIG_SOFTBUTTON_RIGHT_EDGE_BOTTOM:
			return config_get_softbutton(tp, key, value);
		default:
			return 1;
	}

	return 0;
}

int
touchpad_config_get(struct touchpad *tp, ...)
{
	va_list args;
	int processed = 1;
	unsigned int key;
	int* value;

	va_start(args, tp);

	key = va_arg(args, unsigned int);
	while (key != TOUCHPAD_CONFIG_NONE) {
		int success;
		value = va_arg(args, int*);
		success = touchpad_config_get_key_value(tp, key, value);
		if (success != 0) {
			if (success < 0)
				processed = -processed;
			break;
		}
		processed++;
		key = va_arg(args, unsigned int);
	}

	va_end(args);

	if (key == TOUCHPAD_CONFIG_NONE)
		processed = 0;
	return processed;
}

void
touchpad_config_set_static_defaults(struct touchpad *tp)
{
	tp->tap.config = tap_defaults;
	tp->scroll.config = scroll_defaults;
	tp->buttons.config = button_defaults_static;
	tp->config = touchpad_defaults;
}

void
touchpad_config_set_dynamic_defaults(struct touchpad *tp)
{
	touchpad_config_set(tp, NULL,
			TOUCHPAD_CONFIG_SOFTBUTTON_RIGHT_EDGE_LEFT, button_defaults_dynamic.right[0],
			TOUCHPAD_CONFIG_SOFTBUTTON_RIGHT_EDGE_RIGHT, button_defaults_dynamic.right[1],
			TOUCHPAD_CONFIG_SOFTBUTTON_RIGHT_EDGE_TOP, button_defaults_dynamic.right[2],
			TOUCHPAD_CONFIG_SOFTBUTTON_RIGHT_EDGE_BOTTOM, button_defaults_dynamic.right[3],
			TOUCHPAD_CONFIG_NONE);
}

void
touchpad_config_set_defaults(struct touchpad *tp)
{
	touchpad_config_set_static_defaults(tp);
	touchpad_config_set_dynamic_defaults(tp);
}

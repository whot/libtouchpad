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

#define ARRAY_LENGTH(_arr) (sizeof(_arr)/sizeof(_arr[0]))
#define ARRAY_FOR_EACH(_arr, _elem) \
	for (int i = 0; (_elem = &_arr[i]) && i < ARRAY_LENGTH(_arr); i++)

#ifdef min
#undef min
#endif
#define min(a, b) ({ \
	typeof(a) _a = a; \
	typeof(b) _b = b; \
	_a < _b ? _a : _b;})
#ifdef max
#undef max
#endif
#define max(a, b) ({ \
	typeof(a) _a = a; \
	typeof(b) _b = b; \
	_a > _b ? _a : _b;})
#define _unlikely_(x) (__builtin_expect(!!(x), 0))
#define _likely_(x) (__builtin_expect(!!(x), 1))

#define log_bug(cond, ...) \
	do { \
		if (_unlikely_(cond)) { \
			touchpad_log("BUG: %s:%d %s() 'if (" # cond ")'\n", __FILE__, __LINE__, __func__); \
			touchpad_log( __VA_ARGS__); \
		} } while(0)

#define log_debug(msg, ...) \
	do { touchpad_log("%s:%d %s() " msg, __FILE__, __LINE__, __func__, __VA_ARGS__); } while(0)

#define arg_require_int_min(arg, min) \
	log_bug(arg < min, "invalid range: %s must be >= %d, is %d\n", #arg, min, arg)
#define arg_require_int_max(arg, max) \
	log_bug(arg > max, "invalid range: %s must be <= %d, is %d\n", #arg, max, arg)
#define arg_require_int_range(arg, min, max) \
	log_bug(arg < min || arg > max, "invalid range: required %d <= %s <= %d, is %d\n", min, #arg, max, arg)

static inline unsigned int
timeval_to_millis(const struct timeval *tv)
{
	return tv->tv_sec * 1000 + (tv->tv_usec / 1000);
}


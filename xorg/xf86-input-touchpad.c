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
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <xorg-server.h>
#include <exevents.h>
#include <xf86Xinput.h>
#include <xserver-properties.h>
#include <libevdev/libevdev.h>
#include <linux/input.h>
#include "touchpad.h"
#include "touchpad-config.h"
#include "touchpad-util.h"

#define TOUCHPAD_MAX_BUTTONS 7 /* three buttons, 4 scroll buttons */
#define TOUCHPAD_NUM_AXES 4 /* x, y, hscroll, vscroll */

struct xf86touchpad {
	char *path;
	struct touchpad *tp;
	OsTimerPtr timer;

	int scroll_vdist;
	int scroll_hdist;
	int scroll_vdist_remainder;
	int scroll_hdist_remainder;

	struct {
		double x;
		double y;
		double x_remainder;
		double y_remainder;
	} scale;
};

static inline struct touchpad*
xf86touchpad(InputInfoPtr pInfo)
{
	return ((struct xf86touchpad*)pInfo->private)->tp;
}

static int
xf86touchpad_on(DeviceIntPtr dev)
{
	InputInfoPtr pInfo = dev->public.devicePrivate;
	struct xf86touchpad *touchpad = pInfo->private;
	struct touchpad *tp = xf86touchpad(pInfo);
	int fd;
	struct libevdev *evdev;

	fd = open(touchpad->path, O_RDONLY|O_NONBLOCK);
	if (fd < 0)
		return !Success;

	pInfo->fd = fd;
	touchpad_change_fd(tp, fd);
	evdev = touchpad_get_device(tp);
	libevdev_set_clock_id(evdev, CLOCK_MONOTONIC);
	xf86AddEnabledDevice(pInfo);
	dev->public.on = TRUE;

	return dev->public.on ? Success : !Success;
}

static int
xf86touchpad_off(DeviceIntPtr dev)
{
	InputInfoPtr pInfo = dev->public.devicePrivate;
	struct touchpad *tp = xf86touchpad(pInfo);

	xf86RemoveEnabledDevice(pInfo);
	touchpad_change_fd(tp, -1);
	close(pInfo->fd);
	pInfo->fd = -1;
	dev->public.on = FALSE;
	return Success;

}

static void
xf86touchpad_ptr_ctl(DeviceIntPtr dev, PtrCtrl *ctl)
{
}


static void
init_button_map(unsigned char *btnmap, size_t size)
{
	int i;

	memset(btnmap, 0, size);
	for (i = 0; i <= TOUCHPAD_MAX_BUTTONS; i++)
		btnmap[i] = i;
}

static void
init_button_labels(Atom *labels, size_t size)
{
	memset(labels, 0, size * sizeof(Atom));
	labels[0] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_LEFT);
	labels[1] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_MIDDLE);
	labels[2] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_RIGHT);
	labels[3] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_UP);
	labels[4] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_DOWN);
	labels[5] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_LEFT);
	labels[6] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_RIGHT);
}

static void
init_axis_labels(Atom *labels, size_t size)
{
	memset(labels, 0, size * sizeof(Atom));
	labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_X);
	labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_Y);
	labels[2] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_HSCROLL);
	labels[3] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_VSCROLL);
}

static int
xf86touchpad_init(DeviceIntPtr dev)
{
	InputInfoPtr pInfo = dev->public.devicePrivate;
	struct xf86touchpad *touchpad = pInfo->private;
	int min, max, res;

	unsigned char btnmap[TOUCHPAD_MAX_BUTTONS + 1];
	Atom btnlabels[TOUCHPAD_MAX_BUTTONS];
	Atom axislabels[TOUCHPAD_NUM_AXES];

	dev->public.on = FALSE;

	init_button_map(btnmap, ARRAY_SIZE(btnmap));
	init_button_labels(btnlabels, ARRAY_SIZE(btnlabels));
	init_axis_labels(axislabels, ARRAY_SIZE(axislabels));

	InitPointerDeviceStruct((DevicePtr)dev, btnmap,
				TOUCHPAD_MAX_BUTTONS,
				btnlabels,
				xf86touchpad_ptr_ctl,
				GetMotionHistorySize(),
				TOUCHPAD_NUM_AXES,
				axislabels);

	touchpad_get_min_max(xf86touchpad(pInfo), ABS_MT_POSITION_X, &min, &max, &res);
	xf86InitValuatorAxisStruct(dev, 0,
			           XIGetKnownProperty(AXIS_LABEL_PROP_REL_X),
				   min, max, res * 1000, 0, res * 1000, Relative);
	touchpad_get_min_max(xf86touchpad(pInfo), ABS_MT_POSITION_Y, &min, &max, &res);
	xf86InitValuatorAxisStruct(dev, 1,
			           XIGetKnownProperty(AXIS_LABEL_PROP_REL_Y),
				   min, max, res * 1000, 0, res * 1000, Relative);

	touchpad_config_get(xf86touchpad(pInfo),
			    TOUCHPAD_CONFIG_SCROLL_DELTA_HORIZ, &touchpad->scroll_hdist,
			    TOUCHPAD_CONFIG_SCROLL_DELTA_VERT, &touchpad->scroll_vdist,
			    TOUCHPAD_CONFIG_NONE);

	SetScrollValuator(dev, 2, SCROLL_TYPE_HORIZONTAL, touchpad->scroll_hdist, 0);
	SetScrollValuator(dev, 3, SCROLL_TYPE_VERTICAL, touchpad->scroll_vdist, 0);

	return Success;
}

static int
xf86touchpad_device_control(DeviceIntPtr dev, int mode)
{
	int rc = BadValue;

	switch(mode) {
		case DEVICE_INIT:
			rc = xf86touchpad_init(dev);
			break;
		case DEVICE_ON:
			rc = xf86touchpad_on(dev);
			break;
		case DEVICE_OFF:
		case DEVICE_CLOSE:
			rc = xf86touchpad_off(dev);
			break;
	}

	return rc;
}

static void
xf86touchpad_motion(struct touchpad *tp, void *userdata, int x, int y)
{
	InputInfoPtr pInfo = userdata;
	DeviceIntPtr dev = pInfo->dev;

	xf86PostMotionEvent(dev, Relative, 0, 2, x, y);
}

static void
xf86touchpad_button(struct touchpad *tp, void *userdata, unsigned int button /* linux/input.h */, bool is_press)
{
	InputInfoPtr pInfo = userdata;
	DeviceIntPtr dev = pInfo->dev;

	switch(button) {
		case BTN_LEFT: button = 1; break;
		case BTN_MIDDLE: button = 2; break;
		case BTN_RIGHT: button = 3; break;
		default: /* no touchpad actually has those buttons */
			return;
	}

	xf86PostButtonEvent(dev, Relative, button, is_press, 0, 0);
}

static void
xf86touchpad_tap(struct touchpad *tp, void *userdata, unsigned int fingers, bool is_press)
{
	unsigned int button;

	switch(fingers) {
		case 1: button = BTN_LEFT; break;
		case 2: button = BTN_RIGHT; break;
		case 3: button = BTN_MIDDLE; break;
	}

	xf86touchpad_button(tp, userdata, button, is_press);
}


static void
xf86touchpad_scroll(struct touchpad *tp, void *userdata,
		    enum touchpad_scroll_direction direction, double units)
{
	InputInfoPtr pInfo = userdata;
	DeviceIntPtr dev = pInfo->dev;
	struct xf86touchpad *touchpad = pInfo->private;
	int first;

	switch(direction) {
		case TOUCHPAD_SCROLL_HORIZONTAL:
			first = 2;
			units = units * touchpad->scroll_hdist + touchpad->scroll_hdist_remainder;
			touchpad->scroll_hdist_remainder = (int)units % touchpad->scroll_hdist;
			break;
		case TOUCHPAD_SCROLL_VERTICAL:
			first = 3;
			units = units * touchpad->scroll_vdist + touchpad->scroll_vdist_remainder;
			touchpad->scroll_vdist_remainder = (int)units % touchpad->scroll_vdist;
			break;
	}

	xf86PostMotionEvent(dev, Relative, first, 1, (int)units);
}

static CARD32
timer_func(OsTimerPtr timer, CARD32 now, pointer userdata)
{
	InputInfoPtr pInfo = userdata;
	struct touchpad *tp = xf86touchpad(pInfo);
	touchpad_handle_events(tp, userdata, now);
	return 0;
}

static int
xf86touchpad_register_timer(struct touchpad *tp, void *userdata, unsigned int now, unsigned int ms)
{
	InputInfoPtr pInfo = userdata;
	struct xf86touchpad *touchpad = pInfo->private;

	touchpad->timer = TimerSet(touchpad->timer, 0, ms, timer_func, pInfo);
	return 0;
}

static const struct touchpad_interface xf86touchpad_interface = {
	.motion = xf86touchpad_motion,
	.button = xf86touchpad_button,
	.scroll = xf86touchpad_scroll,
	.tap = xf86touchpad_tap,
	.register_timer = xf86touchpad_register_timer
};

static void
xf86touchpad_read_input(InputInfoPtr pInfo)
{
	struct touchpad *tp = xf86touchpad(pInfo);
	unsigned int millis = GetTimeInMillis();

	touchpad_handle_events(tp, pInfo, millis);
}

static bool xf86touchpad_apply_config(InputInfoPtr pInfo,
				      struct touchpad *tp)
{

	const struct lookup {
		const char *name; /* xorg.conf option name */
		enum touchpad_config_parameter key;
	} options[] = {
		{ "MaxTapTime", TOUCHPAD_CONFIG_TAP_TIMEOUT },
		{ "MaxTapMove", TOUCHPAD_CONFIG_TAP_MOVE_THRESHOLD },
		{ "VertScrollDelta", TOUCHPAD_CONFIG_SCROLL_DELTA_VERT },
		{ "VertScrollHoriz", TOUCHPAD_CONFIG_SCROLL_DELTA_HORIZ },
	};
	const struct lookup *opt;
	enum touchpad_config_parameter scroll_methods = TOUCHPAD_SCROLL_NONE;
	bool b;

	pInfo->options = xf86ReplaceIntOption(pInfo->options, "ConstantDeceleration", 6);

	ARRAY_FOR_EACH(options, opt) {
		int value = xf86SetIntOption(pInfo->options, opt->name, INT_MAX);
		if (value != INT_MAX)
			if (touchpad_config_set(tp, NULL, opt->key, value,
						TOUCHPAD_CONFIG_NONE) != 0)
				return false;
	}

	b = xf86SetBoolOption(pInfo->options, "VertTwoFingerScroll", true);
	if (b)
		scroll_methods |= TOUCHPAD_SCROLL_TWOFINGER_HORIZONTAL;
	b = xf86SetBoolOption(pInfo->options, "HorizTwoFingerScroll", true);
	if (b)
		scroll_methods |= TOUCHPAD_SCROLL_TWOFINGER_VERTICAL;
	return touchpad_config_set(tp, NULL,
				   TOUCHPAD_CONFIG_SCROLL_METHOD, scroll_methods,
				   TOUCHPAD_CONFIG_NONE) == 0;
}

static bool xf86touchpad_calc_scale(struct xf86touchpad *touchpad)
{
	int hres, vres;

	if (touchpad_get_min_max(touchpad->tp, ABS_MT_POSITION_X, NULL,
				 NULL, &hres) != 0 ||
	    touchpad_get_min_max(touchpad->tp, ABS_MT_POSITION_Y, NULL,
				 NULL, &vres) != 0)
		return false;

	touchpad->scale.x = hres/(double)vres;
	touchpad->scale.y = 1;

	return true;

}

static void
xf86touchpad_error_log(const char *format, va_list args)
{
	ErrorFSigSafe("----- DRIVER BUG ----\n");
	xorg_backtrace();
	VErrorFSigSafe(format, args);
	ErrorFSigSafe("---------------------\n");
}

static void
xf86touchpad_log(struct touchpad *tp,
		 enum touchpad_log_priority priority,
		 void *data,
		 const char *format, va_list args)
{
	InputInfoPtr pInfo = data;
	MessageType type;
	int verbosity;

	switch(priority) {
		case TOUCHPAD_LOG_BUG:
		case TOUCHPAD_LOG_ERROR:
			type = X_ERROR;
			verbosity = -1;
			break;
		case TOUCHPAD_LOG_DEBUG:
			type = X_INFO;
			verbosity = 4;
			break;
		case TOUCHPAD_LOG_INFO:
			type = X_INFO;
			verbosity = 6;
			break;
		default:
			return;
	}

	LogMessageVerbSigSafe(type, verbosity, "%s: ", pInfo->name);
	LogVMessageVerbSigSafe(type, verbosity, format, args);
}

static int xf86touchpad_pre_init(InputDriverPtr drv,
				 InputInfoPtr pInfo,
				 int flags)
{
	int rc;
	struct xf86touchpad *driver_data = NULL;
	struct touchpad *tp = NULL;
	char *device;

	pInfo->fd = -1;
	pInfo->type_name = XI_TOUCHPAD;
	pInfo->device_control = xf86touchpad_device_control;
	pInfo->read_input = xf86touchpad_read_input;
	pInfo->control_proc = NULL;
	pInfo->switch_mode = NULL;

	driver_data = zalloc(sizeof(*driver_data));
	if (!driver_data)
		goto fail;

	device = xf86SetStrOption(pInfo->options, "Device", NULL);
	if (!device)
		goto fail;

	pInfo->fd = open(device, O_RDONLY|O_NONBLOCK);
	if (pInfo->fd == -1) {
		xf86IDrvMsg(pInfo, X_ERROR, "Opening %s failed with %s\n", device, strerror(errno));
		goto fail;
	}
	rc = touchpad_new_from_fd(pInfo->fd, &tp);
	if (rc != 0) {
		xf86IDrvMsg(pInfo, X_ERROR, "Creating a touchpad for %s failed with %s\n", device, strerror(-rc));
		goto fail;
	}

	touchpad_set_error_log_func(xf86touchpad_error_log);
	touchpad_set_log_func(tp, xf86touchpad_log, pInfo);
	touchpad_set_interface(tp, &xf86touchpad_interface);
	touchpad_change_fd(tp, -1);

	if (!xf86touchpad_apply_config(pInfo, tp))
		goto fail;

	/* empty timer, processing is in the signal handler so we can't
	 * create it there */
	driver_data->timer = TimerSet(NULL, 0, 0, NULL, NULL);
	pInfo->private = driver_data;
	driver_data->tp = tp;
	driver_data->path = device;

	if (!xf86touchpad_calc_scale(driver_data))
		goto fail;

	return Success;

fail:
	if (driver_data && driver_data->timer)
		TimerFree(driver_data->timer);
	close(pInfo->fd);
	free(device);
	free(driver_data);
	return BadValue;
}

static void
xf86touchpad_uninit(InputDriverPtr drv,
		    InputInfoPtr pInfo,
		    int flags)
{
	struct xf86touchpad *touchpad = pInfo->private;
	if (touchpad) {
		struct touchpad *tp = xf86touchpad(pInfo);
		touchpad_free(tp);
		TimerFree(touchpad->timer);
		free(touchpad->path);
		free(touchpad);
		pInfo->private = NULL;
	}
}


InputDriverRec xf86touchpad_driver = {
	.driverVersion	= 1,
	.driverName	= "touchpad",
	.PreInit	= xf86touchpad_pre_init,
	.UnInit		= xf86touchpad_uninit,
};

static XF86ModuleVersionInfo xf86touchpad_version_info = {
	"touchpad",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
	PACKAGE_VERSION_MAJOR, PACKAGE_VERSION_MINOR, PACKAGE_VERSION_PATCHLEVEL,
	ABI_CLASS_XINPUT,
	ABI_XINPUT_VERSION,
	MOD_CLASS_XINPUT,
	{0, 0, 0, 0}
};

static pointer
xf86touchpad_setup_proc(pointer module, pointer options, int *errmaj, int *errmin)
{
	xf86AddInputDriver(&xf86touchpad_driver, module, 0);
	return module;
}

_X_EXPORT XF86ModuleData touchpadModuleData = {
	.vers		= &xf86touchpad_version_info,
	.setup		= &xf86touchpad_setup_proc,
	.teardown	= NULL
};


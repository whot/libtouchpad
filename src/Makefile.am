AM_CPPFLAGS = $(LIBEVDEV_CFLAGS)

lib_LTLIBRARIES = libtouchpad.la
libtouchpad_la_SOURCES = \
	touchpad.c \
	touchpad-events.c \
	touchpad-filter.c \
	touchpad-tap.c \
	touchpad-int.h

libtouchpad_la_LIBADD = $(LIBEVDEV_LIBS)

AM_CPPFLAGS = $(LIBEVDEV_CFLAGS) -I$(top_srcdir)/include

lib_LTLIBRARIES = libtouchpad.la
libtouchpad_la_SOURCES = \
	touchpad.c \
	touchpad-config.h \
	touchpad-config.c \
	touchpad-button.c \
	touchpad-phys-button.c \
	touchpad-events.c \
	touchpad-filter.c \
	touchpad-tap.c \
	touchpad-scroll.c \
	touchpad-int.h \
	touchpad-util.h

libtouchpad_la_LIBADD = $(LIBEVDEV_LIBS)

libtouchpadincludedir = $(includedir)/libtouchpad-1.0/
libtouchpadinclude_HEADERS = touchpad-config.h touchpad.h

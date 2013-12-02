bin_PROGRAMS = touchpad-monitor

touchpad_monitor_SOURCES = touchpad-monitor.c 
touchpad_monitor_LDADD = ../src/libtouchpad.la $(LIBEVDEV_LIBS)
touchpad_monitor_LDFLAGS = -static
AM_CPPFLAGS = -I$(top_srcdir)/src $(LIBEVDEV_CFLAGS)

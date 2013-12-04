bin_PROGRAMS = touchpad-monitor touchpad-edge-detector

AM_CPPFLAGS = -I$(top_srcdir)/src $(LIBEVDEV_CFLAGS)

touchpad_monitor_SOURCES = touchpad-monitor.c 
touchpad_monitor_LDADD = ../src/libtouchpad.la $(LIBEVDEV_LIBS)
touchpad_monitor_LDFLAGS = -static

touchpad_edge_detector_SOURCES = touchpad-edge-detector.c
touchpad_edge_detector_LDADD =  $(LIBEVDEV_LIBS)
touchpad_edge_detector_LDFLAGS = -static

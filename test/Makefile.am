if BUILD_TESTS
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src $(CHECK_CFLAGS) $(LIBEVDEV_CFLAGS)
TEST_LIBS = libtptest.la $(CHECK_LIBS) $(LIBEVDEV_LIBS) $(top_builddir)/src/libtouchpad.la

noinst_LTLIBRARIES = libtptest.la
libtptest_la_SOURCES = \
	tptest.h \
	tptest-int.h \
	tptest.c \
	tptest-bcm5974.h \
	tptest-bcm5974.c \
	tptest-synaptics.h \
	tptest-synaptics.c \
	tptest-synaptics-non-mt.h \
	tptest-synaptics-non-mt.c

noinst_PROGRAMS = test-tap test-config test-scroll test-device test-events test-buttons test-build-pedantic

TESTS = $(noinst_PROGRAMS)

test_tap_SOURCES = test-tap.c
test_tap_LDADD = $(TEST_LIBS)
test_tap_LDFLAGS = -static

test_config_SOURCES = test-config.c
test_config_LDADD = $(TEST_LIBS)
test_config_LDFLAGS = -static

test_scroll_SOURCES = test-scroll.c
test_scroll_LDADD = $(TEST_LIBS)
test_scroll_LDFLAGS = -static

test_device_SOURCES = test-device.c
test_device_LDADD = $(TEST_LIBS)
test_device_LDFLAGS = -static

test_buttons_SOURCES = test-buttons.c
test_buttons_LDADD = $(TEST_LIBS)
test_buttons_LDFLAGS = -static

test_events_SOURCES = test-events.c
test_events_LDADD = $(TEST_LIBS)
test_events_LDFLAGS = -static

# build-test only
test_build_pedantic_SOURCES = test-build-pedantic.c
test_build_pedantic_CFLAGS = $(AM_CPPFLAGS) -pedantic -Werror

endif

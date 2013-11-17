if BUILD_TESTS
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src $(CHECK_CFLAGS) $(LIBEVDEV_CFLAGS)
TEST_LIBS = libtest-common.la $(CHECK_LIBS) $(LIBEVDEV_LIBS) $(top_builddir)/src/libtouchpad.la

noinst_LTLIBRARIES = libtest-common.la
libtest_common_la_SOURCES = \
	test-common.h \
	test-common.c

noinst_PROGRAMS = test-tap test-config

TESTS = $(noinst_PROGRAMS)

test_tap_SOURCES = test-tap.c
test_tap_LDADD = $(TEST_LIBS)
test_tap_LDFLAGS = -static

test_config_SOURCES = test-config.c
test_config_LDADD = $(TEST_LIBS)
test_config_LDFLAGS = -static

endif

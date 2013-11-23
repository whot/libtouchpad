if BUILD_TESTS
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src $(CHECK_CFLAGS) $(LIBEVDEV_CFLAGS)
TEST_LIBS = libtptest.la $(CHECK_LIBS) $(LIBEVDEV_LIBS) $(top_builddir)/src/libtouchpad.la

noinst_LTLIBRARIES = libtptest.la
libtptest_la_SOURCES = \
	tptest.h \
	tptest.c

noinst_PROGRAMS = test-tap test-config test-build-pedantic

TESTS = $(noinst_PROGRAMS)

test_tap_SOURCES = test-tap.c
test_tap_LDADD = $(TEST_LIBS)
test_tap_LDFLAGS = -static

test_config_SOURCES = test-config.c
test_config_LDADD = $(TEST_LIBS)
test_config_LDFLAGS = -static

# build-test only
test_build_pedantic_SOURCES = test-build-pedantic.c
test_build_pedantic_CFLAGS = $(AM_CPPFLAGS) -Wpedantic -Werror

endif

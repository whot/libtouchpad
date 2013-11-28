#  Copyright 2005 Adam Jackson.
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  on the rights to use, copy, modify, merge, publish, distribute, sub
#  license, and/or sell copies of the Software, and to permit persons to whom
#  the Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice (including the next
#  paragraph) shall be included in all copies or substantial portions of the
#  Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
#  ADAM JACKSON BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
#  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# Process this file with autoconf to produce a configure script

# Initialize Autoconf
AC_PREREQ([2.62])
AC_INIT([libtouchpad],
        [0.0.1])
AC_CONFIG_SRCDIR([src/touchpad.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR(.)
AC_USE_SYSTEM_EXTENSIONS
AM_INIT_AUTOMAKE([foreign dist-xz no-dist-gzip])
AM_SILENT_RULES([yes])

AC_PROG_LIBTOOL
AC_PROG_CC
LT_PREREQ([2.2])
LT_INIT
LT_PATH_LD

PKG_CHECK_MODULES(LIBEVDEV, [libevdev >= 0.4])

AC_PATH_PROG(DOXYGEN, [doxygen])
if test "x$DOXYGEN" = "x"; then
	AC_MSG_WARN([doxygen not found - required for documentation])
	have_doxygen="no"
else
	have_doxygen="yes"
fi
AM_CONDITIONAL([HAVE_DOXYGEN], [test "x$have_doxygen" = "xyes"])

AC_ARG_ENABLE(xorg,
	      AS_HELP_STRING([--enable-xorg], [Build xorg driver (default=yes)]),
	      [build_xorg="$enableval"],
	      [build_xorg="yes"])

AM_CONDITIONAL(BUILD_XORG, [test "$build_xorg" = "yes"])
if test "$build_xorg" = "yes"; then
	m4_ifndef([XORG_MACROS_VERSION],
		  [m4_fatal([must install xorg-macros 1.17 or later before running autoconf/autogen])])
	XORG_MACROS_VERSION(1.17)
	XORG_DEFAULT_OPTIONS

	PKG_CHECK_MODULES(XORG, [inputproto >= 2.2] [xorg-server >= 1.12] xproto $REQUIRED_MODULES)
	# Store the list of server defined optional extensions in REQUIRED_MODULES
	m4_ifndef([XORG_DRIVER_CHECK_EXT],
		  [m4_fatal([must install xorg-server development files before running autoconf/autogen])])
	XORG_DRIVER_CHECK_EXT(RANDR, randrproto)


	# X Server SDK location is required to install touchpad header files
	# This location is also relayed in the xorg-touchpad.pc file
	sdkdir=`$PKG_CONFIG --variable=sdkdir xorg-server`
	AC_SUBST([sdkdir])

	# Define a configure option for an alternate input module directory
	AC_ARG_WITH(xorg-module-dir,
		    AC_HELP_STRING([--with-xorg-module-dir=DIR],
				   [Default xorg module directory [[default=$libdir/xorg/modules]]]),
		    [moduledir="$withval"],
		    [moduledir="$libdir/xorg/modules"])
	inputdir=${moduledir}/input
	AC_SUBST(inputdir)

	# Define a configure option for an alternate X Server configuration directory
	sysconfigdir=`$PKG_CONFIG --variable=sysconfigdir xorg-server`
	AC_ARG_WITH(xorg-conf-dir,
		    AC_HELP_STRING([--with-xorg-conf-dir=DIR],
				   [Default xorg.conf.d directory [[default=from $PKG_CONFIG xorg-server]]]),
		    [configdir="$withval"],
		    [configdir="$sysconfigdir"])
	AC_SUBST(configdir)

	# Workaround overriding sdkdir to be able to create a tarball when user has no
	# write permission in sdkdir. See DISTCHECK_CONFIGURE_FLAGS in Makefile.am
	AC_ARG_WITH([sdkdir], [], [sdkdir="$withval"])
fi # with_xorg

AC_CONFIG_FILES([Makefile
		 doc/Makefile
		 src/Makefile
		 test/Makefile
		 tools/Makefile
		 doc/libtouchpad.doxygen
		 libtouchpad.pc
		 xorg/Makefile
		 xorg/xorg-touchpad.pc])
AC_OUTPUT

AC_MSG_RESULT([
	       Build xorg backend:	${build_xorg}
	      ])


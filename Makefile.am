ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
PRINT_DIRECTORY_FLAGS_1=
PRINT_DIRECTORY_FLAGS_0=--no-print-directory
PRINT_DIRECTORY_FLAGS_=$(PRINT_DIRECTORY_FLAGS_$(AM_DEFAULT_VERBOSITY))
AM_MAKEFLAGS = $(PRINT_DIRECTORY_FLAGS_$(V))

SUBDIRS = src test tools

if BUILD_XORG
SUBDIRS += xorg
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtouchpad.pc
EXTRA_DIST = libtouchpad.pc.in

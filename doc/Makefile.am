EXTRA_DIST = touchpad-tap-state-machine.svg

if HAVE_DOXYGEN
all-local: doxygen

doxygen: libtouchpad.doxygen
	doxygen $<

clean-local:
	rm -rf html

.PHONY: doxygen

doc_src= $(shell find html -type f -printf "html/%P\n")
EXTRA_DIST += $(doc_src) libtouchpad.doxygen 
endif

dist_noinst_DATA = libtouchpad.doxygen.in

# make sure doc was built before running dist
dist-hook:
	@test -f $(distdir)/html/index.html || (\
		echo "******************************************************" && \
		echo "Couldn't find documentation files, refusing make dist." && \
		echo "Install doxygen to build documentation for tarball." && \
		echo "******************************************************" && \
		test )

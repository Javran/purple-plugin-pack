EXTRA_DIST = \
	plugins.cfg

stressdir = $(PURPLE_LIBDIR)

stress_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

stress_LTLIBRARIES = stress.la

stress_la_SOURCES = \
	stress.c

stress_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

splitterdir = $(PURPLE_LIBDIR)

splitter_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

splitter_LTLIBRARIES = splitter.la

splitter_la_SOURCES = \
	splitter.c

splitter_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


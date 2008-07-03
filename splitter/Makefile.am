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
	$(CAIRO_LIBS) \
	$(GLIB_LIBS) \
	$(PANGO_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(CAIRO_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PANGO_CFLAGS) \
	$(PURPLE_CFLAGS)


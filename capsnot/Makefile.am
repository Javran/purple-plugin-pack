EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

capsnotdir = $(PURPLE_LIBDIR)

capsnot_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

capsnot_LTLIBRARIES = capsnot.la

capsnot_la_SOURCES = \
	capsnot.c

capsnot_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


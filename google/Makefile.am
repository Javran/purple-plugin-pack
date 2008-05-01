EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

googledir = $(PURPLE_LIBDIR)

google_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

google_LTLIBRARIES = google.la

google_la_SOURCES = \
	google.c

google_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


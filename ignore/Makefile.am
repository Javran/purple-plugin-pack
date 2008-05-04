EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

ignoredir = $(PURPLE_LIBDIR)

ignore_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

ignore_LTLIBRARIES = ignore.la

ignore_la_SOURCES = \
	ignore.c

ignore_la_LIBADD = \
	$(PURPLE_LIBS) \
	$(GLIB_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(GLIB_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)

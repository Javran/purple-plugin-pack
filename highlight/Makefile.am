EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

highlightdir = $(PURPLE_LIBDIR)

highlight_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

highlight_LTLIBRARIES = highlight.la

highlight_la_SOURCES = \
	highlight.c

highlight_la_LIBADD = \
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

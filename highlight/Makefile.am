EXTRA_DIST = .pidgin-plugin Makefile.mingw .incomplete .build

highlightdir = $(PIDGIN_LIBDIR)

highlight_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

highlight_LTLIBRARIES = highlight.la

highlight_la_SOURCES = \
	highlight.c

highlight_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GTK_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS)

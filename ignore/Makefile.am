EXTRA_DIST = .build .purple-plugin Makefile.mingw

ignoredir = $(PIDGIN_LIBDIR)

ignore_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

ignore_LTLIBRARIES = ignore.la

ignore_la_SOURCES = \
	ignore.c

ignore_la_LIBADD = \
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

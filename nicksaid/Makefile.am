EXTRA_DIST = .build .pidgin-plugin Makefile.mingw

nicksaiddir = $(PIDGIN_LIBDIR)

nicksaid_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

nicksaid_LTLIBRARIES = nicksaid.la

nicksaid_la_SOURCES = \
	nicksaid.c

nicksaid_la_LIBADD = \
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

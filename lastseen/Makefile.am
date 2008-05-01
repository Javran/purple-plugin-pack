EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

lastseendir = $(PIDGIN_LIBDIR)

lastseen_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

lastseen_LTLIBRARIES = lastseen.la

lastseen_la_SOURCES = \
	lastseen.c

lastseen_la_LIBADD = \
	$(GTK_LIBS) \
	$(PIDGIN_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(GTK_CFLAGS)


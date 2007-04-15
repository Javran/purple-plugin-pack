EXTRA_DIST = .plugin .build Makefile.mingw

lastseendir = $(GAIM_LIBDIR)/pidgin

lastseen_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

lastseen_LTLIBRARIES = lastseen.la

lastseen_la_SOURCES = \
	lastseen.c

lastseen_la_LIBADD = \
	$(GTK_LIBS) \
	$(GAIM_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/pidgin/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(GTK_CFLAGS)


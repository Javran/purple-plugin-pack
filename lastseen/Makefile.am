EXTRA_DIST = .plugin .build Makefile.mingw

lastseendir = $(GAIM_LIBDIR)/gaim

lastseen_la_LDFLAGS = -module -avoid-version

if PLUGINS

lastseen_LTLIBRARIES = lastseen.la

lastseen_la_SOURCES = \
	lastseen.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(GTK_CFLAGS)


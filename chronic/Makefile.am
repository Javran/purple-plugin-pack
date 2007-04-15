EXTRA_DIST = .plugin

chronicdir=$(GAIM_LIBDIR)/libpurple

chronic_la_LDFLAGS = -module -avoid-version

chronic_LTLIBRARIES = chronic.la

chronic_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GAIM_LIBS)

chronic_la_SOURCES = chronic.c

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/libpurple/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GLIB_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


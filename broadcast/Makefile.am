EXTRA_DIST = \
			.plugin \
			.abusive \
			Makefile.mingw

broadcastdir = $(GAIM_LIBDIR)/libpurple

broadcast_la_LDFLAGS = -module -avoid-version

if PLUGINS

broadcast_LTLIBRARIES = broadcast.la

broadcast_la_SOURCES = \
	broadcast.c

broadcast_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GAIM_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/libpurple/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


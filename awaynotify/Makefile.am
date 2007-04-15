EXTRA_DIST = .plugin

plugindir=$(GAIM_LIBDIR)/libpurple

awaynotify_la_LDFLAGS = -module -avoid-version

plugin_LTLIBRARIES = awaynotify.la

awaynotify_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GAIM_LIBS)

awaynotify_la_SOURCES = awaynotify.c

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/libpurple/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GLIB_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

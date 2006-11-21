EXTRA_DIST = .plugin

awaynotifydir=$(GAIM_LIBDIR)/gaim

awaynotify_la_LDFLAGS = -module -avoid-version

awaynotify_LTLIBRARIES = awaynotify.la

awaynotify_LIBADD = \
	$(GLIB_LIBS) \
	$(GAIM_LIBS)

awaynotify_la_SOURCES = awaynotify.c

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GLIB_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

EXTRA_DIST = .purple-plugin .incomplete

plugindir=$(PURPLE_LIBDIR)

awaynotify_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

plugin_LTLIBRARIES = awaynotify.la

awaynotify_la_SOURCES = awaynotify.c

awaynotify_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	$(GLIB_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)

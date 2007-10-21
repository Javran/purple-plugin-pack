EXTRA_DIST = .incomplete .pidgin-plugin

buddytimedir = $(PIDGIN_LIBDIR)

buddytime_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

buddytime_LTLIBRARIES = buddytime.la

buddytime_la_SOURCES = \
	buddytime.c

buddytime_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GLIB_LIBS) \
	$(GTK_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS)

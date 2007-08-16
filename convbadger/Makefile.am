EXTRA_DIST =
	.build \
	.pidgin-plugin

convbadgerdir = $(PIDGIN_LIBDIR)

convbadger_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

convbadger_LTLIBRARIES = convbadger.la

convbadger_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GLIB_LIBS)

convbadger_la_SOURCES = convbadger.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS)


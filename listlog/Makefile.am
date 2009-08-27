EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

listlogdir = $(PIDGIN_LIBDIR)

listlog_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

listlog_LTLIBRARIES = listlog.la

listlog_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GLIB_LIBS)

listlog_la_SOURCES = listlog.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS)


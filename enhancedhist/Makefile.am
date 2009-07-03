EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

enhancedhistdir = $(PIDGIN_LIBDIR)

enhancedhist_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

enhancedhist_LTLIBRARIES = enhancedhist.la

enhancedhist_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GLIB_LIBS)

enhancedhist_la_SOURCES = enhancedhist.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS)


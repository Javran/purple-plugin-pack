EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

infopanedir = $(PIDGIN_LIBDIR)

infopane_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

infopane_LTLIBRARIES = infopane.la

infopane_la_SOURCES = \
	infopane.c

infopane_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GTK_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS)

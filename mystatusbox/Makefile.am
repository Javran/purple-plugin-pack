EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

mystatusboxdir = $(PIDGIN_LIBDIR)

mystatusbox_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

mystatusbox_LTLIBRARIES = mystatusbox.la

mystatusbox_la_SOURCES = \
	mystatusbox.c

mystatusbox_la_LIBADD = \
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

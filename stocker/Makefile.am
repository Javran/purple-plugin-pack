EXTRA_DIST = \
	.incomplete \
	.pidgin-plugin \
	gtkticker.h \
	stocker_prefs.h \
	Makefile.mingw

stockerdir = $(PIDGIN_LIBDIR)

stocker_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

stocker_LTLIBRARIES = stocker.la

stocker_la_SOURCES = \
	gtkticker.c \
	stocker.c \
	stocker_prefs.c

stocker_la_LIBADD = \
	$(GTK_LIBS) \
	$(PIDGIN_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(GTK_CFLAGS)


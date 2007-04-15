EXTRA_DIST = \
	.plugin \
	gtkticker.h \
	stocker_prefs.h \
	Makefile.mingw

stockerdir = $(GAIM_LIBDIR)/pidgin

stocker_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

stocker_LTLIBRARIES = stocker.la

stocker_la_SOURCES = \
	gtkticker.c \
	stocker.c \
	stocker_prefs.c

stocker_la_LIBADD = \
	$(GTK_LIBS) \
	$(GAIM_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/pidgin/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(GTK_CFLAGS)


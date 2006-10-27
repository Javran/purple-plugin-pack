EXTRA_DIST = \
	.plugin \
	gtkticker.h \
	stocker_prefs.h \
	Makefile.mingw

stockerdir = $(GAIM_LIBDIR)/gaim

stocker_la_LDFLAGS = -module -avoid-version
stocker_la_LIBADD = $(GTK_LIBS)

if PLUGINS

stocker_LTLIBRARIES = stocker.la

stocker_la_LIBADD = \
	$(GAIM_LIBS)

stocker_la_SOURCES = \
	gtkticker.c \
	stocker.c \
	stocker_prefs.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(GTK_CFLAGS)


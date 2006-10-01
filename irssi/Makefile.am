EXTRA_DIST = .plugin .build Makefile.mingw

irssidir = $(GAIM_LIBDIR)/gaim

irssi_la_LDFLAGS = -module -avoid-version
irssi_la_LIBADD = $(GLIB_LIBS)

if PLUGINS

irssi_LTLIBRARIES = irssi.la

irssi_la_SOURCES = \
	irssi.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS)


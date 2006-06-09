EXTRA_DIST = .plugin .build Makefile.mingw

irssidatedir = $(GAIM_LIBDIR)/gaim

irssidate_la_LDFLAGS = -module -avoid-version

if PLUGINS

irssidate_LTLIBRARIES = irssidate.la

irssidate_la_SOURCES = \
	irssidate.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(GLIB_CFLAGS)


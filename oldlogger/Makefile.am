EXTRA_DIST = .plugin .build Makefile.mingw

oldloggerdir = $(GAIM_LIBDIR)/gaim

oldlogger_la_LDFLAGS = -module -avoid-version

if PLUGINS

oldlogger_LTLIBRARIES = oldlogger.la

oldlogger_la_SOURCES = \
	oldlogger.c

oldlogger_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GAIM_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


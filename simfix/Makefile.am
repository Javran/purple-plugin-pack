EXTRA_DIST = .build .plugin Makefile.mingw

simfixdir = $(GAIM_LIBDIR)/gaim

simfix_la_LDFLAGS = -module -avoid-version $(GLIB_LIBS)

if PLUGINS

simfix_LTLIBRARIES = simfix.la

simfix_la_SOURCES = \
	simfix.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


EXTRA_DIST = .build .plugin Makefile.mingw

simfixdir = $(GAIM_LIBDIR)/libpurple

simfix_la_LDFLAGS = -module -avoid-version

if PLUGINS

simfix_LTLIBRARIES = simfix.la

simfix_la_SOURCES = \
	simfix.c

simfix_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GAIM_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/libpurple/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


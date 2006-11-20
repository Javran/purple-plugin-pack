EXTRA_DIST = .plugin .build

napsterdir = $(GAIM_LIBDIR)/gaim

napster_la_LDFLAGS = -module -avoid-version

if PLUGINS

napster_LTLIBRARIES = napster.la

napster_la_SOURCES = \
	napster.c

napster_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GAIM_LIBS)

napsterpixdir=$(GAIM_DATADIR)/pixmaps/gaim/status/default

napsterpix_DATA=napster.png

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


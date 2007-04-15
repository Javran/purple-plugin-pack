EXTRA_DIST = .plugin

napsterdir = $(GAIM_LIBDIR)/libpurple

napster_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

napster_LTLIBRARIES = napster.la

napster_la_SOURCES = \
	napster.c

napster_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GAIM_LIBS)

napsterpixdir=$(GAIM_DATADIR)/pixmaps/libpurple/status/default

napsterpix_DATA=napster.png

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/libpurple/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


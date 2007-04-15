EXTRA_DIST = .plugin .build Makefile.mingw

dicedir = $(GAIM_LIBDIR)/libpurple

dice_la_LDFLAGS = -module -avoid-version

if PLUGINS

dice_LTLIBRARIES = dice.la

dice_la_SOURCES = \
	dice.c

dice_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GAIM_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/libpurple/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


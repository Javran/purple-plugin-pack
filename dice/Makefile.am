EXTRA_DIST = .plugin .build Makefile.mingw

dicedir = $(GAIM_LIBDIR)/gaim

dice_la_LDFLAGS = -module -avoid-version $(GLIB_LIBS)

if PLUGINS

dice_LTLIBRARIES = dice.la

dice_la_SOURCES = \
	dice.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


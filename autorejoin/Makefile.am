EXTRA_DIST = .plugin Makefile.mingw

autorejoindir = $(GAIM_LIBDIR)/pidgin

autorejoin_la_LDFLAGS = -module -avoid-version

if PLUGINS

autorejoin_LTLIBRARIES = autorejoin.la

autorejoin_la_LIBADD = \
	$(GTK_LIBS)

autorejoin_la_SOURCES = \
	autorejoin.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/pidgin/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

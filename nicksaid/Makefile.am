EXTRA_DIST = .build .plugin Makefile.mingw

nicksaiddir = $(GAIM_LIBDIR)/gaim

nicksaid_la_LDFLAGS = -module -avoid-version

if PLUGINS

nicksaid_LTLIBRARIES = nicksaid.la

nicksaid_la_LIBADD = \
	$(GTK_LIBS)

nicksaid_la_SOURCES = \
	nicksaid.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

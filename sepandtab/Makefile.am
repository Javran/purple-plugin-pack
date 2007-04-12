EXTRA_DIST = .plugin .build Makefile.mingw

sepandtabdir = $(GAIM_LIBDIR)/pidgin

sepandtab_la_LDFLAGS = -module -avoid-version

if PLUGINS

sepandtab_LTLIBRARIES = sepandtab.la

sepandtab_la_SOURCES = \
	sepandtab.c

sepandtab_la_LIBADD = \
	$(GAIM_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/pidgin/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(GTK_CFLAGS)


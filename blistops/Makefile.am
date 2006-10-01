EXTRA_DIST = .plugin .build Makefile.mingw

blistopsdir = $(GAIM_LIBDIR)/gaim

blistops_la_LDFLAGS = -module -avoid-version $(GTK_LIBS)

if PLUGINS

blistops_LTLIBRARIES = blistops.la

blistops_la_SOURCES = \
	blistops.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(GTK_CFLAGS)


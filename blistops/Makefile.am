EXTRA_DIST = .plugin .build Makefile.mingw

blistopsdir = $(GAIM_LIBDIR)/gaim

blistops_la_LDFLAGS = -module -avoid-version

if PLUGINS

blistops_LTLIBRARIES = blistops.la

blistops_la_SOURCES = \
	blistops.c

blistops_la_LIBADD = \
	$(GTK_LIBS) \
	$(GAIM_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(GTK_CFLAGS)


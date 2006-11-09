EXTRA_DIST = .build .plugin Makefile.mingw

autoacceptdir = $(GAIM_LIBDIR)/gaim

autoaccept_la_LDFLAGS = -module -avoid-version

if PLUGINS

autoaccept_LTLIBRARIES = autoaccept.la

autoaccept_la_SOURCES = \
	autoaccept.c

autoaccept_la_LIBADD = \
	$(GAIM_LIBS) \
	$(GTK_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

EXTRA_DIST = .build .plugin Makefile.mingw

autoacceptdir = $(GAIM_LIBDIR)/gaim

autoaccept_la_LDFLAGS = -module -avoid-version

if PLUGINS

autoaccept_LTLIBRARIES = autoaccept.la

autoaccept_la_LIBADD = \
	$(GTK_LIBS)

autoaccept_la_SOURCES = \
	autoaccept.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

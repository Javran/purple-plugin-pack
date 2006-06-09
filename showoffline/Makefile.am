EXTRA_DIST = .plugin .build Makefile.mingw

showofflinedir = $(GAIM_LIBDIR)/gaim

showoffline_la_LDFLAGS = -module -avoid-version

if PLUGINS

showoffline_LTLIBRARIES = showoffline.la

showoffline_la_SOURCES = \
	showoffline.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


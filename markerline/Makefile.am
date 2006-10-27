EXTRA_DIST = .build .plugin Makefile.mingw

markerlinedir = $(GAIM_LIBDIR)/gaim

markerline_la_LDFLAGS = -module -avoid-version

if PLUGINS

markerline_LTLIBRARIES = markerline.la

markerline_la_LIBADD = \
	$(GAIM_LIBS) \
	$(GTK_LIBS)

markerline_la_SOURCES = \
	markerline.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

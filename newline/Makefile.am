EXTRA_DIST = .plugin .build Makefile.mingw

newlinedir = $(GAIM_LIBDIR)/gaim

newline_la_LDFLAGS = -module -avoid-version

if PLUGINS

newline_LTLIBRARIES = newline.la

newline_la_SOURCES = \
	newline.c

newline_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GAIM_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


EXTRA_DIST = .plugin .build Makefile.mingw

showofflinedir = $(GAIM_LIBDIR)/libpurple

showoffline_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

showoffline_LTLIBRARIES = showoffline.la

showoffline_la_SOURCES = \
	showoffline.c

showoffline_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GAIM_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/libpurple/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


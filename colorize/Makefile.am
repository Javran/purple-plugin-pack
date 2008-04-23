EXTRA_DIST = .purple-plugin .build Makefile.mingw

colorizedir = $(PURPLE_LIBDIR)

colorize_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

colorize_LTLIBRARIES = colorize.la

colorize_la_SOURCES = \
	colorize.c

colorize_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


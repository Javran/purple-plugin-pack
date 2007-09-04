EXTRA_DIST = .purple-plugin .abusive Makefile.mingw

findipdir = $(PURPLE_LIBDIR)

findip_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

findip_LTLIBRARIES = findip.la

findip_la_SOURCES = \
	findip.c

findip_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


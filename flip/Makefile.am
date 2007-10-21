EXTRA_DIST = .purple-plugin .build Makefile.mingw

flipdir = $(PURPLE_LIBDIR)

flip_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

flip_LTLIBRARIES = flip.la

flip_la_SOURCES = \
	flip.c

flip_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS) \
	$(GLIB_CFLAGS)


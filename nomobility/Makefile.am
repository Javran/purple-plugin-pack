EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

nomobilitydir = $(PURPLE_LIBDIR)

nomobility_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

nomobility_LTLIBRARIES = nomobility.la

nomobility_la_SOURCES = \
	nomobility.c

nomobility_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


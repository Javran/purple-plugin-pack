EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

autoreplydir = $(PURPLE_LIBDIR)

autoreply_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

autoreply_LTLIBRARIES = autoreply.la

autoreply_la_SOURCES = \
	autoreply.c

autoreply_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


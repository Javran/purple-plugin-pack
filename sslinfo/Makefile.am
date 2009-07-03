EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

sslinfodir = $(PURPLE_LIBDIR)

sslinfo_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

sslinfo_LTLIBRARIES = sslinfo.la

sslinfo_la_SOURCES = \
	sslinfo.c

sslinfo_la_LIBADD = \
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


EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

oldloggerdir = $(PURPLE_LIBDIR)

oldlogger_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

oldlogger_LTLIBRARIES = oldlogger.la

oldlogger_la_SOURCES = \
	oldlogger.c

oldlogger_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


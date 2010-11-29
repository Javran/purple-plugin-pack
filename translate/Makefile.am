EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

translatedir = $(PURPLE_LIBDIR)

translate_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

translate_LTLIBRARIES = translate.la

translate_la_SOURCES = \
	translate.c

translate_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


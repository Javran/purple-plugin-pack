EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

dewysiwygificationdir = $(PURPLE_LIBDIR)

dewysiwygification_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

dewysiwygification_LTLIBRARIES = dewysiwygification.la

dewysiwygification_la_SOURCES = \
	dewysiwygification.c

dewysiwygification_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


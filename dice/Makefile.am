EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

dicedir = $(PURPLE_LIBDIR)

dice_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

dice_LTLIBRARIES = dice.la

dice_la_SOURCES = \
	dice.c

dice_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


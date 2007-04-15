EXTRA_DIST = .build .purple-plugin Makefile.mingw

simfixdir = $(PURPLE_LIBDIR)

simfix_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

simfix_LTLIBRARIES = simfix.la

simfix_la_SOURCES = \
	simfix.c

simfix_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


EXTRA_DIST = .build .purple-plugin Makefile.mingw

irc_moredir = $(PURPLE_LIBDIR)

irc_more_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

irc_more_LTLIBRARIES = irc-more.la

irc_more_la_SOURCES = \
	irc-more.c

irc_more_la_LIBADD = \
	$(PURPLE_LIBS) \
	$(GLIB_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(GLIB_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)

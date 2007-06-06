EXTRA_DIST = .pidgin-plugin Makefile.mingw .incomplete

irc_moredir = $(PIDGIN_LIBDIR)

irc_more_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

irc_more_LTLIBRARIES = irc-more.la

irc_more_la_SOURCES = \
	irc-more.c

irc_more_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GTK_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS)

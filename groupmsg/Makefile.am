EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

groupmsgdir = $(PURPLE_LIBDIR)

groupmsg_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

groupmsg_LTLIBRARIES = groupmsg.la

groupmsg_la_SOURCES = \
	groupmsg.c

groupmsg_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


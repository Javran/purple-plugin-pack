EXTRA_DIST = .purple-plugin .abusive Makefile.mingw

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
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


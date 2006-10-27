EXTRA_DIST = \
			.plugin \
			.abusive \
			Makefile.mingw

groupmsgdir = $(GAIM_LIBDIR)/gaim

groupmsg_la_LDFLAGS = -module -avoid-version
groupmsg_la_LIBADD = $(GLIB_LIBS)

if PLUGINS

groupmsg_LTLIBRARIES = groupmsg.la

groupmsg_la_LIBADD = \
	$(GAIM_LIBS)

groupmsg_la_SOURCES = \
	groupmsg.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


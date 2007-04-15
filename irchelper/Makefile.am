EXTRA_DIST = .plugin .build Makefile.mingw

irchelperdir = $(GAIM_LIBDIR)/libpurple

irchelper_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

irchelper_LTLIBRARIES = irchelper.la

irchelper_la_SOURCES = \
	irchelper.c

irchelper_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GAIM_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/libpurple/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


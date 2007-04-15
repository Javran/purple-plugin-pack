EXTRA_DIST = .purple-plugin .build Makefile.mingw

irchelperdir = $(PURPLE_LIBDIR)

irchelper_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

irchelper_LTLIBRARIES = irchelper.la

irchelper_la_SOURCES = \
	irchelper.c

irchelper_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


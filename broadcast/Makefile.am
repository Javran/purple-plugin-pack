EXTRA_DIST = \
			.plugin \
			.abusive \
			Makefile.mingw

broadcastdir = $(PURPLE_LIBDIR)

broadcast_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

broadcast_LTLIBRARIES = broadcast.la

broadcast_la_SOURCES = \
	broadcast.c

broadcast_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


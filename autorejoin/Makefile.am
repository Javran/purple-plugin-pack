EXTRA_DIST = .purple-plugin Makefile.mingw

autorejoindir = $(PURPLE_LIBDIR)/purple

autorejoin_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

autorejoin_LTLIBRARIES = autorejoin.la

autorejoin_la_LIBADD = \
	$(GTK_LIBS)

autorejoin_la_SOURCES = \
	autorejoin.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)/purple/\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DLOCALEDIR=\"$(PURPLE_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)

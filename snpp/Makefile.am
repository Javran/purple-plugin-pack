EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

libsnppdir = $(PURPLE_LIBDIR)

libsnpp_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

libsnpp_LTLIBRARIES = libsnpp.la

libsnpp_la_SOURCES = snpp.c

libsnpp_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)

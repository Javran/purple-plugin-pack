EXTRA_DIST = .plugin .build Makefile.mingw

flipdir = $(GAIM_LIBDIR)/libpurple

flip_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

flip_LTLIBRARIES = flip.la

flip_la_SOURCES = \
	flip.c

flip_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GAIM_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/libpurple/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


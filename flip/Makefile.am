EXTRA_DIST = .plugin .build Makefile.mingw

flipdir = $(GAIM_LIBDIR)/gaim

flip_la_LDFLAGS = -module -avoid-version
flip_la_LIBADD = $(GLIB_LIBS)

if PLUGINS

flip_LTLIBRARIES = flip.la

flip_la_SOURCES = \
	flip.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


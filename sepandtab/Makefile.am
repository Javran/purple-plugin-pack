EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

sepandtabdir = $(PIDGIN_LIBDIR)

sepandtab_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

sepandtab_LTLIBRARIES = sepandtab.la

sepandtab_la_SOURCES = \
	sepandtab.c

sepandtab_la_LIBADD = \
	$(PIDGIN_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(GTK_CFLAGS)


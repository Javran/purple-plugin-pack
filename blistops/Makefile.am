EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

blistopsdir = $(PIDGIN_LIBDIR)

blistops_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

blistops_LTLIBRARIES = blistops.la

blistops_la_SOURCES = \
	blistops.c

blistops_la_LIBADD = \
	$(GTK_LIBS) \
	$(PIDGIN_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(GTK_CFLAGS)


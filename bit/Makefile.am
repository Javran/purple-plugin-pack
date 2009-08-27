EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

bitdir = $(PIDGIN_LIBDIR)

bit_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

bit_LTLIBRARIES = bit.la

bit_la_SOURCES = \
	bit.c

bit_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GTK_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS)

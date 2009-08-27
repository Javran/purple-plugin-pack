EXTRA_DIST = \
	plugins.cfg

plonkersdir = $(PIDGIN_LIBDIR)

plonkers_la_LDFLAGS = \
	-module \
	-avoid-version

if HAVE_PIDGIN

plonkers_LTLIBRARIES = plonkers.la

plonkers_la_SOURCES = \
	plonkers.c

plonkers_la_LIBADD = \
	$(GTK_LIBS) \
	$(PIDGIN_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(PIDGIN_CFLAGS)

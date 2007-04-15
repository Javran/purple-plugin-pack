EXTRA_DIST = .pidgin-plugin .build

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
	$(DEBUG_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(PIDGIN_CFLAGS)

EXTRA_DIST = \
	.plugin \
	.build

plonkersdir = $(GAIM_LIBDIR)/pidgin

plonkers_la_LDFLAGS = \
	-module \
	-avoid-version

if HAVE_PIDGIN

plonkers_LTLIBRARIES = plonkers.la

plonkers_la_SOURCES = \
	plonkers.c

plonkers_la_LIBADD = \
	$(GTK_LIBS) \
	$(GAIM_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/pidgin/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	$(DEBUG_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GAIM_CFLAGS)

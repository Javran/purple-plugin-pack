EXTRA_DIST = \
	.plugin \
	.build

plonkersdir = $(GAIM_LIBDIR)/gaim

plonkers_la_LDFLAGS = \
	-module \
	-avoid-version

plonkers_la_LIBADD = $(GTK_LIBS)

if PLUGINS

plonkers_LTLIBRARIES = plonkers.la

plonkers_la_LIBADD = \
	$(GAIM_LIBS)

plonkers_la_SOURCES = \
	plonkers.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	$(DEBUG_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GAIM_CFLAGS)

EXTRA_DIST = \
	.plugin \
	.build

plonkersdir = $(GAIM_LIBDIR)/gaim

plonkers_la_LDFLAGS = \
	-module \
	-avoid-version \
	$(GTK_LIBS)

if PLUGINS

plonkers_LTLIBRARIES = plonkers.la

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

EXTRA_DIST = \
	.plugin \
	chronic.wav

chronicdir=$(GAIM_LIBDIR)/gaim

chronic_la_LDFLAGS = -module -avoid-version

chronic_LTLIBRARIES = chronic.la

chronic_la_SOURCES = chronic.c

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GLIB_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


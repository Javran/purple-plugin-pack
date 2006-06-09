EXTRA_DIST = .plugin .build Makefile.mingw

sslinfodir = $(GAIM_LIBDIR)/gaim

sslinfo_la_LDFLAGS = -module -avoid-version

if PLUGINS

sslinfo_LTLIBRARIES = sslinfo.la

sslinfo_la_SOURCES = \
	sslinfo.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(GLIB_CFLAGS)


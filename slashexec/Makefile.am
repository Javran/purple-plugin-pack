EXTRA_DIST = \
	.plugin \
	.build \
	Makefile.mingw

slashexecdir = $(GAIM_LIBDIR)/gaim

slashexec_la_LDFLAGS = -module -avoid-version $(GLIB_LIBS)

if PLUGINS

slashexec_LTLIBRARIES = slashexec.la

slashexec_la_SOURCES = slashexec.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(GLIB_CFLAGS)


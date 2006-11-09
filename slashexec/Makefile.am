EXTRA_DIST = \
	.plugin \
	.build \
	Makefile.mingw

slashexecdir = $(GAIM_LIBDIR)/gaim

slashexec_la_LDFLAGS = -module -avoid-version

if PLUGINS

slashexec_LTLIBRARIES = slashexec.la

slashexec_la_SOURCES = slashexec.c

slashexec_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GAIM_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(GLIB_CFLAGS)


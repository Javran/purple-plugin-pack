EXTRA_DIST = \
	.plugin \
	.build \
	Makefile.mingw

slashexecdir = $(GAIM_LIBDIR)/libpurple

slashexec_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

slashexec_LTLIBRARIES = slashexec.la

slashexec_la_SOURCES = slashexec.c

slashexec_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GAIM_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/libpurple/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(GLIB_CFLAGS)


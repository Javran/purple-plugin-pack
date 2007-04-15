EXTRA_DIST = \
	.plugin \
	.build \
	Makefile.mingw

bashdir = $(GAIM_LIBDIR)/libpurple

bash_la_LDFLAGS = -module -avoid-version

bash_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GAIM_LIBS)

if PLUGINS

bash_LTLIBRARIES = bash.la

bash_la_SOURCES = bash.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/libpurple/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(GLIB_CFLAGS)


EXTRA_DIST = \
	.plugin \
	.build \
	Makefile.mingw

bashdir = $(GAIM_LIBDIR)/gaim

bash_la_LDFLAGS = -module -avoid-version
bash_la_LIBADD = $(GLIB_LIBS)

if PLUGINS

bash_LTLIBRARIES = bash.la

bash_la_SOURCES = bash.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(GLIB_CFLAGS)


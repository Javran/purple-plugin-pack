EXTRA_DIST = .plugin Makefile.mingw

buzzdir = $(GAIM_LIBDIR)/gaim

buzz_la_LDFLAGS = -module -avoid-version

if PLUGINS

buzz_LTLIBRARIES = buzz.la

buzz_la_LIBADD = \
	$(GTK_LIBS)

buzz_la_SOURCES = \
	im_buzz.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(GTK_CFLAGS)


EXTRA_DIST = .plugin Makefile.mingw .build

convcolorsdir = $(GAIM_LIBDIR)/gaim

convcolors_la_LDFLAGS = -module -avoid-version

if PLUGINS

convcolors_LTLIBRARIES = convcolors.la

convcolors_la_LIBADD = \
	$(GAIM_LIBS) \
	$(GTK_LIBS)

convcolors_la_SOURCES = \
	convcolors.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

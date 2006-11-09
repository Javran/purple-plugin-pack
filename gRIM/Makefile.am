EXTRA_DIST = .plugin Makefile.mingw .build

gRIMdir = $(GAIM_LIBDIR)/gaim

gRIM_la_LDFLAGS = -module -avoid-version

if PLUGINS

gRIM_LTLIBRARIES = gRIM.la

gRIM_la_SOURCES = \
	gRIM.c

gRIM_la_LIBADD = \
	$(GAIM_LIBS) \
	$(GTK_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

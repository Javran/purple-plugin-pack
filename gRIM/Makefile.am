EXTRA_DIST = .pidgin-plugin Makefile.mingw .build

gRIMdir = $(PIDGIN_LIBDIR)

gRIM_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

gRIM_LTLIBRARIES = gRIM.la

gRIM_la_SOURCES = \
	gRIM.c

gRIM_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GTK_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS)

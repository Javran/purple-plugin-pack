EXTRA_DIST = \
	.build \
	.plugin \
	Makefile.mingw \
	album.h \
	album-ui.h

albumdir = $(GAIM_LIBDIR)/pidgin

album_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

album_LTLIBRARIES = album.la

album_la_SOURCES = \
	album.c \
	album-ui.c

album_la_LIBADD = \
	$(GAIM_LIBS) \
	$(GTK_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/pidgin/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

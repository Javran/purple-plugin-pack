HEADER_FILES = \
	album.h \
	album-ui.h

EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg \
	$(HEADER_FILES)

albumdir = $(PIDGIN_LIBDIR)

album_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

album_LTLIBRARIES = album.la

album_la_SOURCES = \
	album.c \
	album-ui.c

album_la_LIBADD = \
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

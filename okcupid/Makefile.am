PIXMAPS = \
	16/okcupid.png \
	22/okcupid.png \
	48/okcupid.png
	
EXTRA_DIST=\
	Makefile.mingw \
	plugins.cfg \
	$(PIXMAPS)

okcupiddir = $(PURPLE_LIBDIR)

okcupid_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE
if HAVE_JSON_GLIB

okcupid_LTLIBRARIES = okcupid.la

okcupid_la_SOURCES = \
	okcupid.c \
	okcupid.h \
	okc_blist.c \
	okc_blist.h \
	okc_connection.c \
	okc_connection.h \
	okc_messages.c \
	okc_messages.h

okcupid_la_LIBADD = \
	$(GLIB_LIBS) \
	$(JSON_GLIB_LIBS) \
	$(PURPLE_LIBS)

if HAVE_PIDGIN
okcupidpix16dir=$(PIDGIN_PIXMAPSDIR)/protocols/16
okcupidpix16_DATA=16/okcupid.png

okcupidpix22dir=$(PIDGIN_PIXMAPSDIR)/protocols/22
okcupidpix22_DATA=22/okcupid.png

okcupidpix48dir=$(PIDGIN_PIXMAPSDIR)/protocols/48
okcupidpix48_DATA=48/okcupid.png

endif

endif
endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	$(DEBUG_CFLAGS) \
	$(JSON_GLIB_CFLAGS) \
	$(PURPLE_CFLAGS)

bullshit:
	echo $(DESTDIR)

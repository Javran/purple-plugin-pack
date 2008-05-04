PIXMAPS = \
	16/napster.png \
	22/napster.png \
	48/napster.png
	
EXTRA_DIST=\
	Makefile.mingw \
	plugins.cfg \
	$(PIXMAPS)

napsterdir = $(PURPLE_LIBDIR)

napster_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

napster_LTLIBRARIES = napster.la

napster_la_SOURCES = \
	napster.c

napster_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

if HAVE_PIDGIN
napsterpix16dir=$(PIDGIN_PIXMAPSDIR)/protocols/16
napsterpix16_DATA=16/napster.png

napsterpix22dir=$(PIDGIN_PIXMAPSDIR)/protocols/22
napsterpix22_DATA=22/napster.png

napsterpix48dir=$(PIDGIN_PIXMAPSDIR)/protocols/48
napsterpix48_DATA=48/napster.png

endif

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)

bullshit:
	echo $(DESTDIR)

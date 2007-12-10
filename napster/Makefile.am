EXTRA_DIST=\
	.purple-plugin \
	.incomplete \
	Makefile.mingw \
	napster16x16.png \
	napster22x22.png \
	napster48x48.png

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
napsterpix16x16dir=$(PIDGIN_PIXMAPSDIR)/protocols/16
napsterpix16x16_DATA=napster16x16.png

napsterpix22x22dir=$(PIDGIN_PIXMAPSDIR)/protocols/22
napsterpix22x22_DATA=napster22x22.png

napsterpix48x48dir=$(PIDGIN_PIXMAPSDIR)/protocols/48
napsterpix48x48_DATA=napster48x48.png

install-data-hook:
	mv $(PIDGIN_PIXMAPSDIR)/protocols/16/napster16x16.png $(PIDGIN_PIXMAPSDIR)//protocols/16/napster.png
	mv $(PIDGIN_PIXMAPSDIR)/protocols/22/napster22x22.png $(PIDGIN_PIXMAPSDIR)//protocols/22/napster.png
	mv $(PIDGIN_PIXMAPSDIR)/protocols/48/napster48x48.png $(PIDGIN_PIXMAPSDIR)//protocols/48/napster.png

endif

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


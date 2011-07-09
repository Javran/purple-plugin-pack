#PIXMAPS = \
#	16/omegle.png \
#	22/omegle.png \
#	48/omegle.png
	
EXTRA_DIST=\
	Makefile.mingw \
	plugins.cfg
#	$(PIXMAPS) #add \ to the line above and uncomment this line if we get pixmaps

omegledir = $(PURPLE_LIBDIR)

omegle_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE
if HAVE_JSON_GLIB

omegle_LTLIBRARIES = omegle.la

omegle_la_SOURCES = \
	omegle.h \
	omegle.c \
	om_connection.h \
	om_connection.c

omegle_la_LIBADD = \
	$(GLIB_LIBS) \
	$(JSON_GLIB_LIBS) \
	$(PURPLE_LIBS)

# uncomment the lines below if we get pixmaps
#if HAVE_PIDGIN
#omeglepix16dir=$(PIDGIN_PIXMAPSDIR)/protocols/16
#omeglepix16_DATA=16/omegle.png
#
#omeglepix22dir=$(PIDGIN_PIXMAPSDIR)/protocols/22
#omeglepix22_DATA=22/omegle.png
#
#omeglepix48dir=$(PIDGIN_PIXMAPSDIR)/protocols/48
#omeglepix48_DATA=48/omegle.png
#
#endif

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

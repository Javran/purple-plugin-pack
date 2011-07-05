#PIXMAPS = \
#	16/ning.png \
#	22/ning.png \
#	48/ning.png
	
EXTRA_DIST=\
	Makefile.mingw \
	plugins.cfg
#	$(PIXMAPS) #add \ to the line above and uncomment this line if we get pixmaps

ningdir = $(PURPLE_LIBDIR)

ning_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

ning_LTLIBRARIES = ning.la

ning_la_SOURCES = \
	ning.h \
	ning_chat.h \
	ning_connection.h \
	ning.c \
	ning_chat.c \
	ning_connection.c

ning_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

# uncomment the lines below if we get pixmaps
#if HAVE_PIDGIN
#ningpix16dir=$(PIDGIN_PIXMAPSDIR)/protocols/16
#ningpix16_DATA=16/ning.png
#
#ningpix22dir=$(PIDGIN_PIXMAPSDIR)/protocols/22
#ningpix22_DATA=22/ning.png
#
#ningpix48dir=$(PIDGIN_PIXMAPSDIR)/protocols/48
#ningpix48_DATA=48/ning.png
#
#endif

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)

bullshit:
	echo $(DESTDIR)

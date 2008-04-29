ICONS = \
	next.png \
	pause.png \
	play.png \
	previous.png \
	stop.png \
	xmms.png

EXTRA_DIST = \
	$(ICONS) \
	plugins.cfg

xmmsremoteconfpixdir = $(PIDGIN_PIXMAPSDIR)/plugin_pack/xmmsremote
xmmsremoteconfpix_DATA = $(ICONS)

xmmsremotedir = $(PIDGIN_LIBDIR)

xmmsremote_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN
if USE_XMMS
xmmsremote_LTLIBRARIES = xmmsremote.la

xmmsremote_la_SOURCES = \
	xmmsremote.c

xmmsremote_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GTK_LIBS) \
	$(XMMS_LIBS)

endif
endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(XMMS_CFLAGS)

EXTRA_DIST = .build .plugin

SUBDIRS = pixmaps

xmmsremotedir = $(GAIM_LIBDIR)/gaim

xmmsremote_la_LDFLAGS = -module -avoid-version

if PLUGINS
if USE_XMMS
xmmsremote_LTLIBRARIES = xmmsremote.la

xmmsremote_la_SOURCES = \
	xmmsremote.c

xmmsremote_la_LIBADD = \
	$(GAIM_LIBS) \
	$(GTK_LIBS) \
	$(XMMS_LIBS)

endif
endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(XMMS_CFLAGS)

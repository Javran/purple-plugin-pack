EXTRA_DIST = .build .plugin

SUBDIRS = pixmaps

xmmsremotedir = $(GAIM_LIBDIR)/gaim

xmmsremote_la_LDFLAGS = -module -avoid-version $(XMMS_LIBS)

if PLUGINS
if USE_XMMS
xmmsremote_LTLIBRARIES = xmmsremote.la

xmmsremote_la_LIBADD = \
	$(GTK_LIBS) 

xmmsremote_la_SOURCES = \
	xmmsremote.c
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

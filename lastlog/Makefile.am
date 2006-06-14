EXTRA_DIST = .build .plugin

lastlogdir = $(GAIM_LIBDIR)/gaim

lastlog_la_LDFLAGS = -module -avoid-version $(XMMS_LIBS)

if PLUGINS
if USE_XMMS
lastlog_LTLIBRARIES = lastlog.la

lastlog_la_LIBADD = \
	$(GTK_LIBS) 

lastlog_la_SOURCES = \
	lastlog.c
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

EXTRA_DIST = .plugin

buddytimedir = $(GAIM_LIBDIR)/pidgin

buddytime_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

buddytime_LTLIBRARIES = buddytime.la

buddytime_la_SOURCES = \
	buddytime.c

buddytime_la_LIBADD = \
	$(GAIM_LIBS) \
	$(GLIB_LIBS) \
	$(GTK_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/pidgin/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

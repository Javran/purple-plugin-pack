EXTRA_DIST = .build .plugin Makefile.mingw

mystatusboxdir = $(GAIM_LIBDIR)/pidgin

mystatusbox_la_LDFLAGS = -module -avoid-version

if PLUGINS

mystatusbox_LTLIBRARIES = mystatusbox.la

mystatusbox_la_SOURCES = \
	mystatusbox.c

mystatusbox_la_LIBADD = \
	$(GAIM_LIBS) \
	$(GTK_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/pidgin/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

EXTRA_DIST = .plugin Makefile.mingw

mystatusboxdir = $(GAIM_LIBDIR)/gaim

mystatusbox_la_LDFLAGS = -module -avoid-version $(GTK_LIBS)

if PLUGINS

mystatusbox_LTLIBRARIES = mystatusbox.la

mystatusbox_la_LIBADD = \
	$(GTK_LIBS)

mystatusbox_la_SOURCES = \
	mystatusbox.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

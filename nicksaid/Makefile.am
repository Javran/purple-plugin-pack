EXTRA_DIST = .build .plugin Makefile.mingw

nicksaiddir = $(GAIM_LIBDIR)/pidgin

nicksaid_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

nicksaid_LTLIBRARIES = nicksaid.la

nicksaid_la_SOURCES = \
	nicksaid.c

nicksaid_la_LIBADD = \
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

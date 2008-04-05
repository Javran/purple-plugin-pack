EXTRA_DIST = .pidgin-plugin .build Makefile.mingw

manualsizedir = $(PIDGIN_LIBDIR)

manualsize_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

manualsize_LTLIBRARIES = manualsize.la

manualsize_la_SOURCES = \
	manualsize.c

manualsize_la_LIBADD = \
	$(GTK_LIBS) \
	$(PIDGIN_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(GTK_CFLAGS)


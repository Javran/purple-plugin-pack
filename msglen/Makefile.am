EXTRA_DIST = .pidgin-plugin .incomplete Makefile.mingw

msglendir = $(PIDGIN_LIBDIR)

msglen_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

msglen_LTLIBRARIES = msglen.la

msglen_la_SOURCES = \
	msglen.c

msglen_la_LIBADD = \
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


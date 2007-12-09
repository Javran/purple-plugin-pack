EXTRA_DIST = \
	.build \
	.pidgin-plugin \
	Makefile.mingw

enhanced_histdir = $(PIDGIN_LIBDIR)

enhanced_hist_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

enhanced_hist_LTLIBRARIES = enhanced_hist.la

enhanced_hist_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GLIB_LIBS)

enhanced_hist_la_SOURCES = enhanced_hist.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS)


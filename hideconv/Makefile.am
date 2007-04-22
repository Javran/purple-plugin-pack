EXTRA_DIST = .plugin Makefile.mingw .build

hideconvdir = $(PIDGIN_LIBDIR)

hideconv_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

hideconv_LTLIBRARIES = hideconv.la

hideconv_la_SOURCES = \
	hideconv.c

hideconv_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GTK_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)/pidgin/\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DLOCALEDIR=\"$(PIDGIN_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS)

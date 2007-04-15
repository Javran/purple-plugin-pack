EXTRA_DIST = .pidgin-plugin .build Makefile.mingw

blistopsdir = $(PIDGIN_LIBDIR)

blistops_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

blistops_LTLIBRARIES = blistops.la

blistops_la_SOURCES = \
	blistops.c

blistops_la_LIBADD = \
	$(GTK_LIBS) \
	$(PIDGIN_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(GTK_CFLAGS)


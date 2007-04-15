EXTRA_DIST = .pidgin-plugin Makefile.mingw .build

difftopicdir = $(PIDGIN_LIBDIR)

difftopic_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

difftopic_LTLIBRARIES = difftopic.la

difftopic_la_SOURCES = \
	difftopic.c

difftopic_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GTK_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)/pidgin/\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS)

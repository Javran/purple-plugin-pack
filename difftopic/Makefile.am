EXTRA_DIST = .plugin Makefile.mingw

difftopicdir = $(GAIM_LIBDIR)/gaim

difftopic_la_LDFLAGS = -module -avoid-version

if PLUGINS

difftopic_LTLIBRARIES = difftopic.la

difftopic_la_LIBADD = \
	$(GTK_LIBS)

difftopic_la_SOURCES = \
	difftopic.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

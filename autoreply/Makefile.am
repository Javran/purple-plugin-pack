EXTRA_DIST = .plugin

autoreplydir = $(GAIM_LIBDIR)/gaim

autoreply_la_LDFLAGS = -module -avoid-version

if PLUGINS

autoreply_LTLIBRARIES = autoreply.la

autoreply_la_LIBADD = \
	$(GTK_LIBS)

autoreply_la_SOURCES = \
	autoreply.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


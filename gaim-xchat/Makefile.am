EXTRA_DIST = .plugin .build

gaim_xchatdir = $(GAIM_LIBDIR)/gaim

gaim_xchat_la_LDFLAGS = -module -avoid-version

if PLUGINS

gaim_xchat_LTLIBRARIES = gaim-xchat.la

gaim_xchat_la_SOURCES = \
	xtext.h	\
	xtext.c	\
	gaim-xchat.c

gaim_xchat_la_LIBADD = \
	$(GAIM_LIBS) \
	$(GTK_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

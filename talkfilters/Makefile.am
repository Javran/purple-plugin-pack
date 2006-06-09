EXTRA_DIST = .build .plugin

talkfiltersdir = $(GAIM_LIBDIR)/gaim

talkfilters_la_LDFLAGS = -module -avoid-version

if PLUGINS
if USE_TALKFILTERS
talkfilters_LTLIBRARIES = talkfilters.la

talkfilters_la_LIBADD = \
	$(GTK_LIBS) \
	$(TALKFILTERS_LIBS)

talkfilters_la_SOURCES = \
	talkfilters.c
endif
endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

EXTRA_DIST = .build .plugin

talkfiltersdir = $(GAIM_LIBDIR)/pidgin

talkfilters_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN
if USE_TALKFILTERS
talkfilters_LTLIBRARIES = talkfilters.la

talkfilters_la_SOURCES = \
	talkfilters.c

talkfilters_la_LIBADD = \
	$(GAIM_LIBS) \
	$(GTK_LIBS) \
	$(TALKFILTERS_LIBS)

endif
endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/pidgin/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

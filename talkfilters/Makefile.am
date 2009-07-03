EXTRA_DIST = \
	plugins.cfg

talkfiltersdir = $(PIDGIN_LIBDIR)

talkfilters_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN
if USE_TALKFILTERS
talkfilters_LTLIBRARIES = talkfilters.la

talkfilters_la_SOURCES = \
	talkfilters.c

talkfilters_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GTK_LIBS) \
	$(TALKFILTERS_LIBS)

endif
endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS)

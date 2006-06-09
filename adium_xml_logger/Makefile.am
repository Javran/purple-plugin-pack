EXTRA_DIST = .plugin Makefile.mingw

adiumxmlloggerdir = $(GAIM_LIBDIR)/gaim

adiumxmllogger_la_LDFLAGS = -module -avoid-version

if PLUGINS

adiumxmllogger_LTLIBRARIES = adiumxmllogger.la

adiumxmllogger_la_SOURCES = \
	adiumxmllogger.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


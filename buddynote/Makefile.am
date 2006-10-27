EXTRA_DIST = .plugin .build Makefile.mingw

buddynotedir = $(GAIM_LIBDIR)/gaim

buddynote_la_LDFLAGS = -module -avoid-version
buddynote_la_LIBADD = $(GLIB_LIBS)

if PLUGINS

buddynote_LTLIBRARIES = buddynote.la

buddynote_la_LIBADD = \
	$(GAIM_LIBS)

buddynote_la_SOURCES = \
	buddynote.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


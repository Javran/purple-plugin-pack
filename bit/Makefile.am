EXTRA_DIST = .plugin Makefile.mingw .build

bitdir = $(GAIM_LIBDIR)/pidgin

bit_la_LDFLAGS = -module -avoid-version

if PLUGINS

bit_LTLIBRARIES = bit.la

bit_la_SOURCES = \
	bit.c

bit_la_LIBADD = \
	$(GAIM_LIBS) \
	$(GTK_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/pidgin/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

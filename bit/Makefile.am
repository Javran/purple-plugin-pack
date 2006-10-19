EXTRA_DIST = .plugin Makefile.mingw .build

bitdir = $(GAIM_LIBDIR)/gaim

bit_la_LDFLAGS = -module -avoid-version

if PLUGINS

bit_LTLIBRARIES = bit.la

bit_la_LIBADD = \
	$(GTK_LIBS)

bit_la_SOURCES = \
	bit.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

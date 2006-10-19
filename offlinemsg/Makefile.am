EXTRA_DIST = .build .plugin Makefile.mingw

offlinemsgdir = $(GAIM_LIBDIR)/gaim

offlinemsg_la_LDFLAGS = -module -avoid-version

if PLUGINS

offlinemsg_LTLIBRARIES = offlinemsg.la

offlinemsg_la_LIBADD = \
	$(GTK_LIBS)

offlinemsg_la_SOURCES = \
	offlinemsg.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

EXTRA_DIST = \
	.build \
	.plugin \
	*.h \
	Makefile.mingw

listhandlerdir = $(GAIM_LIBDIR)/gaim

listhandler_la_LDFLAGS = -module -avoid-version

if PLUGINS

listhandler_LTLIBRARIES = listhandler.la

listhandler_la_SOURCES = \
	aim_blt_files.c \
	gen_xml_files.c \
	lh_util.c \
	migrate.c \
	listhandler.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)


INCLUDES = \
	aim_blt_files.h \
	alias_xml_files.h \
	gen_xml_files.h \
	lh_util.h \
	listhandler.h \
	migrate.h \
	purple_blist_xml.h

EXTRA_DIST=\
	Makefile.mingw \
	plugins.cfg \
	$(INCLUDES)

listhandlerdir = $(PURPLE_LIBDIR)

listhandler_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

listhandler_LTLIBRARIES = listhandler.la

listhandler_la_SOURCES = \
	aim_blt_files.c \
	alias_xml_files.c \
	gen_xml_files.c \
	lh_util.c \
	migrate.c \
	listhandler.c \
	purple_blist_xml.c

listhandler_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


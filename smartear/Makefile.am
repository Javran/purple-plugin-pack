EXTRA_DIST=\
	.build \
	.pidgin-plugin \
	Makefile.mingw \
	smartear.glade \
	interface.c \
	make-interface.sh

smarteardir = $(PIDGIN_LIBDIR)

smartear_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

smartear_LTLIBRARIES = smartear.la

smartear_la_SOURCES = \
	smartear.c \
	smartear-interface.c

smartear_la_LIBADD = \
	$(GTK_LIBS) \
	$(PIDGIN_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(GTK_CFLAGS) \
	$(PIDGIN_CFLAGS)


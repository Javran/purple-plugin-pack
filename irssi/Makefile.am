HEADER_FILES = \
	datechange.h \
	irssi.h \
	lastlog.h \
	layout.h \
	textfmt.h \
	window.h

EXTRA_DIST=\
	Makefile.mingw \
	plugins.cfg \
	$(HEADER_FILES)

irssidir = $(PIDGIN_LIBDIR)

irssi_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

irssi_LTLIBRARIES = irssi.la

irssi_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GLIB_LIBS)

irssi_la_SOURCES = \
	datechange.c \
	irssi.c \
	lastlog.c \
	layout.c \
	textfmt.c \
	window.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS)


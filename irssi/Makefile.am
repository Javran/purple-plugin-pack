EXTRA_DIST=\
	.pidgin-plugin \
	.build \
	Makefile.mingw \
	datechange.h \
	lastlog.h \
	layout.h \
	textfmt.h \
	window.h

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
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS)


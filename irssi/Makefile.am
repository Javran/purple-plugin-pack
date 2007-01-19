EXTRA_DIST=\
	.plugin \
	.build \
	Makefile.mingw \
	datechange.h \
	lastlog.h \
	layout.h \
	textfmt.h \
	window.h

irssidir = $(GAIM_LIBDIR)/gaim

irssi_la_LDFLAGS = -module -avoid-version

if PLUGINS

irssi_LTLIBRARIES = irssi.la

irssi_la_LIBADD = \
	$(GAIM_LIBS) \
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
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS)


HEADER_FILES = \
	callbacks.h \
	ignorance.h \
	ignorance_denizen.h \
	ignorance_internal.h \
	ignorance_level.h \
	ignorance_rule.h \
	ignorance_violation.h \
	interface.h \
	regex.h \
	support.h

EXTRA_DIST = \
	plugins.cfg \
	ignorance.conf \
	$(HEADER_FILES)

ignorancedir = $(PIDGIN_LIBDIR)/pidgin

ignorance_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

ignorance_LTLIBRARIES = \
	ignorance.la

ignorance_confdir = \
	$(sysconfdir)/pidgin

ignorance_conf_DATA = \
	ignorance.conf

ignorance_la_SOURCES = \
	ignorance.c \
	ignorance_level.c \
	ignorance_rule.c \
	ignorance_violation.c \
	ignorance_denizen.c \
	callbacks.c \
	interface.c \
	support.c

ignorance_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GTK_LIBS) \
	$(GLIB_LIBS)

endif

AM_CPPFLAGS = \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DVERSION=\"$(VERSION)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	-DIGNORANCE_CONFDIR=\"$(ignorance_confdir)\" \
	$(DEBUG_CFLAGS) \
	$(GTK_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(PLUGIN_CFLAGS)

ignorance.conf:

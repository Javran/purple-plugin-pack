EXTRA_DIST = \
	.plugin \
	callbacks.h \
	ignorance.conf \
	ignorance.h \
	ignorance_denizen.h \
	ignorance_internal.h \
	ignorance_level.h \
	ignorance_rule.h \
	ignorance_violation.h \
	interface.h \
	regex.h \
	support.h

ignorancedir = $(GAIM_LIBDIR)/gaim

ignorance_la_LDFLAGS = -module -avoid-version

if PLUGINS

ignorance_LTLIBRARIES = \
	ignorance.la

ignorance_confdir = \
	$(sysconfdir)/gaim

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
	$(GAIM_LIBS) \
	$(GTK_LIBS) \
	$(GLIB_LIBS)

endif

AM_CPPFLAGS = \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DVERSION=\"$(VERSION)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	-DIGNORANCE_CONFDIR=\"$(ignorance_confdir)\" \
	$(DEBUG_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(PLUGIN_CFLAGS)

ignorance.conf:

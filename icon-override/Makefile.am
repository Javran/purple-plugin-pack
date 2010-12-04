EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

icon_overridedir = $(PIDGIN_LIBDIR)

icon_override_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

icon_override_LTLIBRARIES = icon_override.la

icon_override_la_SOURCES = \
	icon_override.c

icon_override_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GTK_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)/pidgin/\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DLOCALEDIR=\"$(PIDGIN_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS)

EXTRA_DIST = .purple-plugin .incomplete

chronicdir=$(PURPLE_LIBDIR)

chronic_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

chronic_LTLIBRARIES = chronic.la

chronic_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

chronic_la_SOURCES = chronic.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	$(GLIB_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


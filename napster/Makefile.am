EXTRA_DIST = .purple-plugin .incomplete

napsterdir = $(PURPLE_LIBDIR)

napster_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

napster_LTLIBRARIES = napster.la

napster_la_SOURCES = \
	napster.c

napster_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

napsterpixdir=$(PURPLE_PIXAMPSDIR)/status/default

napsterpix_DATA=napster.png

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


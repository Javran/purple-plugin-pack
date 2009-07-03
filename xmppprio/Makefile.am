EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

xmpppriodir = $(PURPLE_LIBDIR)

xmppprio_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

xmppprio_LTLIBRARIES = xmppprio.la

xmppprio_la_SOURCES = \
	xmppprio.c

xmppprio_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


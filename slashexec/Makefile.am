EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

slashexecdir = $(PURPLE_LIBDIR)

slashexec_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

slashexec_LTLIBRARIES = slashexec.la

slashexec_la_SOURCES = slashexec.c

slashexec_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS) \
	$(GLIB_CFLAGS)


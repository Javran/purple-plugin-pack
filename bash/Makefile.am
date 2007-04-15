EXTRA_DIST = \
	.purple-plugin \
	.build \
	Makefile.mingw

bashdir = $(PURPLE_LIBDIR)

bash_la_LDFLAGS = -module -avoid-version

bash_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

if HAVE_PURPLE

bash_LTLIBRARIES = bash.la

bash_la_SOURCES = bash.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS) \
	$(GLIB_CFLAGS)


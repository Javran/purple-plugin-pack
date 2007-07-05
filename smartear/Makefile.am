EXTRA_DIST= \
	.build \
	.purple-plugin \
	Makefile.mingw

smarteardir = $(PIDGIN_LIBDIR)

smartear_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

smartear_LTLIBRARIES = smartear.la

smartear_la_SOURCES = smartear.c

smartear_la_LIBADD = $(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


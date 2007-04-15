EXTRA_DIST = .purple-plugin .build Makefile.mingw

showofflinedir = $(PURPLE_LIBDIR)

showoffline_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

showoffline_LTLIBRARIES = showoffline.la

showoffline_la_SOURCES = \
	showoffline.c

showoffline_la_LIBADD = \
	$(GLIB_LIBS) \
	$(PURPLE_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)/libpurple/\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


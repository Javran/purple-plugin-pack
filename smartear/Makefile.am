EXTRA_DIST= \
	.build \
	.purple-plugin \
	.pidgin-plugin \
	.finch-plugin \
	Makefile.mingw

smarteardir = $(PIDGIN_LIBDIR)

smartear_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE
smartear_LTLIBRARIES = smartear.la
smartear_la_SOURCES = smartear.c
smartear_la_LIBADD = $(PURPLE_LIBS)
endif

if HAVE_PIDGIN
gtksmartear_LTLIBRARIES = gtksmartear.la
gtksmartear_la_SOURCES = gtksmartear.c
gtksmartear_la_LIBADD = $(PIDGIN_LIBS)
endif

if HAVE_FINCH
gntsmartear_LTLIBRARIES = gntsmartear.la
gntsmartear_la_SOURCES = gntsmartear.c
gntsmartear_la_LIBADD = $(FINCH_LIBS)
endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PURPLE_CFLAGS)


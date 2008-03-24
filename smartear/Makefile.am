EXTRA_DIST= \
	.build \
	.pidgin-plugin \
	.purple-plugin \
	.finch-plugin \
	Makefile.mingw


if HAVE_PURPLE
smarteardir = $(PURPLE_LIBDIR)
smartear_la_LDFLAGS = -module -avoid-version
smartear_la_CPPFLAGS = \
	-DLIBDIR=\"$(PURPLE_LIBDIR)\" \
	-DDATADIR=\"$(PURPLE_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PURPLE_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(PURPLE_CFLAGS)

smartear_LTLIBRARIES = smartear.la
smartear_la_SOURCES = smartear.c
smartear_la_LIBADD = $(PURPLE_LIBS)
endif

if HAVE_PIDGIN
gtksmarteardir = $(PIDGIN_LIBDIR)
gtksmartear_la_LDFLAGS = -module -avoid-version
gtksmartear_la_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIIXMAPSDIR)\" \
	$(GTK_CFLAGS) \
	$(PIDGIN_CFLAGS)

gtksmartear_LTLIBRARIES = gtksmartear.la
gtksmartear_la_SOURCES = gtksmartear.c
gtksmartear_la_LIBADD = \
	$(GTK_LIBS) \
	$(PIDGIN_LIBS) \
	$(PURPLE_LIBS)

endif

if HAVE_FINCH
gntsmarteardir = $(FINCH_LIBDIR)
gntsmartear_la_LDFLAGS = -module -avoid-version
gntsmartear_la_CPPFLAGS = \
	-DLIBDIR=\"$(FINCH_LIBDIR)\" \
	-DDATADIR=\"$(FINCH_DATADIR)\" \
	-DPIXMAPSDIR=\"$(FINCH_PIIXMAPSDIR)\" \
	$(FINCH_CFLAGS) \
	$(GNT_CFLAGS)

gntsmartear_LTLIBRARIES = gntsmartear.la
gntsmartear_la_SOURCES = gntsmartear.c
gntsmartear_la_LIBADD = \
	$(FINCH_LIBS) \
	$(GNT_LIBS) \
	$(PURPLE_LIBS)

endif


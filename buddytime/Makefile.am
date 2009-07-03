EXTRA_DIST = \
	plugins.cfg

noinst_PROGRAMS = recursetest

recursetest_SOURCES = \
	recurse.c \
	recursetest.c

if HAVE_PIDGIN

noinst_PROGRAMS += gtktimezonetest

gtktimezonetest_SOURCES = \
	gtktimezone.c \
	gtktimezonetest.c \
	recurse.c

gtktimezonetest_LDADD = \
	$(GTK_LIBS)

gtkbuddytimedir = $(PIDGIN_LIBDIR)
gtkbuddytime_la_LDFLAGS = -module -avoid-version
gtkbuddytime_LTLIBRARIES = gtkbuddytime.la
gtkbuddytime_la_SOURCES = \
	gtkbuddytime.c
gtkbuddytime_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GLIB_LIBS) \
	$(GTK_LIBS)

endif

buddytimedir = $(PURPLE_LIBDIR)
buddytime_la_LDFLAGS = -module -avoid-version
buddytime_LTLIBRARIES = buddytime.la
buddytime_la_SOURCES = \
	buddytime.c
buddytime_la_LIBADD = \
	$(PURPLE_LIBS) \
	$(GLIB_LIBS)


AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS)

EXTRA_DIST = \
	plugins.cfg

timelogdir = $(PIDGIN_LIBDIR)

timelog_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

timelog_LTLIBRARIES = timelog.la

timelog_la_SOURCES = \
	log-widget.c \
	log-widget.h \
	range-widget.c \
	range-widget.h \
	timelog.c \
	timelog.h

timelog_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GTK_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS)

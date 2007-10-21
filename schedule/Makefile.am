EXTRA_DIST = .build .pidgin-plugin schedule.h

pidgin_scheduledir = $(PIDGIN_LIBDIR)

pidgin_schedule_la_LDFLAGS = -module -avoid-version

pidgin_schedule_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GTK_LIBS)

if HAVE_PIDGIN

pidgin_schedule_LTLIBRARIES = pidgin-schedule.la

pidgin_schedule_la_SOURCES = \
	schedule.c	\
	pidgin-schedule.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS)

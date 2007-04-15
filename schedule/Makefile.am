EXTRA_DIST = .build .pidgin-plugin schedule.h

gaim_scheduledir = $(PIDGIN_LIBDIR)

gaim_schedule_la_LDFLAGS = -module -avoid-version

gaim_schedule_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GTK_LIBS)

if HAVE_PIDGIN

gaim_schedule_LTLIBRARIES = gaim-schedule.la

gaim_schedule_la_SOURCES = \
	schedule.c	\
	gaim-schedule.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS)

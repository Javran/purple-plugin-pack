EXTRA_DIST = \
	.build \
	.plugin \
	schedule.h

gaim_scheduledir = $(GAIM_LIBDIR)/pidgin

gaim_schedule_la_LDFLAGS = -module -avoid-version

gaim_schedule_la_LIBADD = \
	$(GAIM_LIBS) \
	$(GTK_LIBS)

if PLUGINS

gaim_schedule_LTLIBRARIES = gaim-schedule.la

gaim_schedule_la_SOURCES = \
	schedule.c	\
	gaim-schedule.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/pidgin/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

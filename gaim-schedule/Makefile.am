EXTRA_DIST = .plugin

gaim_scheduledir = $(GAIM_LIBDIR)/gaim

gaim_schedule_la_LDFLAGS = -module -avoid-version

if PLUGINS

gaim_schedule_LTLIBRARIES = gaim-schedule.la

gaim_schedule_la_LIBADD = \
	$(GTK_LIBS)

gaim_schedule_la_SOURCES = \
	schedule.c	\
	gaim-schedule.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

EXTRA_DIST = \
	.build \
	.plugin \
	Makefile.mingw

eight_balldir = $(GAIM_LIBDIR)/gaim

eight_ball_la_LDFLAGS = -module -avoid-version $(GLIB_LIBS)

if PLUGINS

eight_ball_LTLIBRARIES = eight_ball.la

eight_ball_la_SOURCES = eight_ball.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(GLIB_CFLAGS)


EXTRA_DIST = \
	.build \
	.plugin \
	Makefile.mingw

eight_balldir = $(GAIM_LIBDIR)/libpurple

eight_ball_la_LDFLAGS = -module -avoid-version

if HAVE_PURPLE

eight_ball_LTLIBRARIES = eight_ball.la

eight_ball_la_SOURCES = eight_ball.c

eight_ball_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GAIM_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/libpurple/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(GLIB_CFLAGS)


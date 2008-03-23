EXTRA_DIST = \
	.build \
	.pidgin-plugin

autoprofiledir = $(PURPLE_LIBDIR)

autoprofile_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

autoprofile_LTLIBRARIES = autoprofile.la

autoprofile_la_SOURCES = \
	autoaway.c \
	autoprofile.c \
	autoprofile.h \
	autoreply.c \
	comp_countdownup.c \
	comp_executable.c \
	comp_http.c \
	comp_logstats.c \
	comp_logstats_gtk.c \
	comp_logstats.h \
	component.c \
	component.h \
	comp_quotation.c \
	comp_rss.c \
	comp_rss.h \
	comp_rss_parser.c \
	comp_rss_xanga.c \
	comp_textfile.c \
	comp_timestamp.c \
	comp_uptime.c \
	gtk_actions.c \
	gtk_away_msgs.c \
	gtk_widget.c \
	Makefile.am \
	preferences.c \
	sizes.h \
	utility.c \
	utility.h \
	utility_rfc822.c \
	widget.c \
	widget.h

listhandler_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GLIB_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS)


EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

if BUILD_SWITCH_SPELL
switchspelldir = $(PIDGIN_LIBDIR)

switchspell_la_LDFLAGS = -module -avoid-version \
	$(GTKSPELL_LIBS)

if HAVE_PIDGIN

switchspell_LTLIBRARIES = switchspell.la

switchspell_la_SOURCES = \
	switchspell.c

switchspell_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GTK_LIBS)

endif # PIDGIN

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(GTKSPELL_CFLAGS)

endif # BUILD_SWITCH_SPELL

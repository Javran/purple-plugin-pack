EXTRA_DIST = \
	Makefile.mingw \
	plugins.cfg

if BUILD_SWITCH_SPELL
switchspelldir = $(PIDGIN_LIBDIR)

switchspell_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

switchspell_LTLIBRARIES = switchspell.la

switchspell_la_SOURCES = \
	switchspell.c

switchspell_la_LIBADD = \
	$(ASPELL_LIBS) \
	$(GTK_LIBS) \
	$(GTKSPELL_LIBS) \
	$(PIDGIN_LIBS)

endif # PIDGIN

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	-DPIXMAPSDIR=\"$(PIDGIN_PIXMAPSDIR)\" \
	$(ASPELL_CFLAGS) \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(GTKSPELL_CFLAGS)

endif # BUILD_SWITCH_SPELL

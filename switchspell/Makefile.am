EXTRA_DIST = .plugin Makefile.mingw .build

if HAVE_GTKSPELL
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
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(GTKSPELL_CFLAGS)

endif # HAVE_GTKSPELL

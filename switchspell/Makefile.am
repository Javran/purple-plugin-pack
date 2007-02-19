EXTRA_DIST = .plugin Makefile.mingw .build

if HAVE_GTKSPELL
switchspelldir = $(GAIM_LIBDIR)/gaim

switchspell_la_LDFLAGS = -module -avoid-version \
	$(GTKSPELL_LIBS)

if PLUGINS

switchspell_LTLIBRARIES = switchspell.la

switchspell_la_SOURCES = \
	switchspell.c

switchspell_la_LIBADD = \
	$(GAIM_LIBS) \
	$(GTK_LIBS)

endif # PLUGINS

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/gaim/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS) \
	$(GTKSPELL_CFLAGS)

endif # HAVE_GTKSPELL

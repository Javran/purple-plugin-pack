EXTRA_DIST = .plugin .build

xchat_chatsdir = $(GAIM_LIBDIR)/pidgin

xchat_chats_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

xchat_chats_LTLIBRARIES = xchat-chats.la

xchat_chats_la_SOURCES = \
	xtext.h	\
	xtext.c	\
	xchat-chats.c

xchat_chats_la_LIBADD = \
	$(GAIM_LIBS) \
	$(GTK_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(GAIM_LIBDIR)/pidgin/\" \
	-DDATADIR=\"$(GAIM_DATADIR)\" \
	-DLOCALEDIR=\"$(GAIM_DATADIR)/locale\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)

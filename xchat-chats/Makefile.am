EXTRA_DIST = .pidgin-plugin .build

xchat_chatsdir = $(PIDGIN_LIBDIR)

xchat_chats_la_LDFLAGS = -module -avoid-version

if HAVE_PIDGIN

xchat_chats_LTLIBRARIES = xchat-chats.la

xchat_chats_la_SOURCES = \
	xtext.h	\
	xtext.c	\
	xchat-chats.c

xchat_chats_la_LIBADD = \
	$(PIDGIN_LIBS) \
	$(GTK_LIBS)

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(PIDGIN_LIBDIR)\" \
	-DDATADIR=\"$(PIDGIN_DATADIR)\" \
	$(GTK_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS)

/*
 * irssi - Implements several irssi features for Gaim
 * Copyright (C) 2005-2006 Gary Kramlich <grim@reaperworld.com>
 * Copyright (C) 2006 John Bailey <rekkanoryo@rekkanoryo.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

/* gpp_config.h provides necessary definitions that help us find/do stuff */
#if HAVE_CONFIG_H
# include "../gpp_config.h"
#endif

/* all Gaim plugins need to define this */
#define GAIM_PLUGINS

/* define these so the plugin info struct way at the bottom is cleaner */
#define PLUGIN_ID			"gtk-plugin_pack-irssi"
#define PLUGIN_NAME			"Irssi Features"
#define PLUGIN_STATIC_NAME	"irssi"
#define PLUGIN_SUMMARY		"Implements features of the irssi IRC client for " \
							"use in Gaim."
#define PLUGIN_DESCRIPTION	"Implements some features of the IRC client irssi " \
							"to be used in Gaim.  It lets you know in all open " \
							"conversations when the day has changed, adds the " \
							"lastlog command, adds the window command, etc.  " \
							"The day changed message is not logged."
#define PLUGIN_AUTHOR		"Gary Kramlich <grim@reaperworld.com>, " \
							"John Bailey <rekkanoryo@rekkanoryo.org, " \
							"Sadrul Habib Chowdhury <sadrul@users.sourceforge.net>"

/* System headers */
#include <glib.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Gaim headers */
#include <conversation.h>
#include <cmds.h>
#include <gtkconv.h>
#include <gtkimhtml.h>
#include <gtkplugin.h>
#include <notify.h>
#include <plugin.h>
#include <util.h>
#include <version.h>

/* Pack/Local headers */
#include "../common/i18n.h"

static guint irssi_datechange_timeout_id = 0;
static gint lastday = 0;

/* these are planned features */
static GaimCmdId irssi_window_cmd = 0;	/* will provide /window */
static GaimCmdId irssi_win_cmd = 0;		/* will provide /win */
static GaimCmdId irssi_layout_cmd = 0;	/* will provide /layout */
static GaimCmdId irssi_lastlog_cmd = 0;	/* will provide /lastlog */

static gint
irssi_datechange_get_day(time_t *t) {
	struct tm *temp;
	
	temp = localtime(t);

	return temp->tm_mday;
}

static gint
irssi_datechange_get_month(time_t *t) {
	struct tm *temp;

	temp = localtime(t);

	return temp->tm_mon;
}

static gboolean
irssi_datechange_timeout_cb(gpointer data) {
	time_t t;
	GList *l;
	gint newday;
	gchar buff[80];
	gchar *message;

	t = time(NULL);
	newday = irssi_datechange_get_day(&t);

	if(newday == lastday)
		return TRUE;

	strftime(buff, sizeof(buff), "%d %b %Y", localtime(&t));
	message = g_strdup_printf(_("Day changed to %s"), buff);

	for(l = gaim_get_conversations(); l; l = l->next) {
		GaimConversation *conv = (GaimConversation *)l->data;

		gaim_conversation_write(conv, NULL, message,
								GAIM_MESSAGE_NO_LOG | GAIM_MESSAGE_SYSTEM | GAIM_MESSAGE_ACTIVE_ONLY,
								t);
		if ((irssi_datechange_get_day(&t) == 1) &&
				(irssi_datechange_get_month(&t) == 0))
		{
			const gchar *new_year = _("Happy New Year");
			if(conv->type == GAIM_CONV_TYPE_IM)
				gaim_conv_im_send(GAIM_CONV_IM(conv), new_year);
			else if(conv->type == GAIM_CONV_TYPE_CHAT)
				gaim_conv_chat_send(GAIM_CONV_CHAT(conv), new_year);
		}
	}

	g_free(message);

	lastday = newday;

	return TRUE;
}

static gboolean
irssi_window_close_cb(GaimConversation *c)
{
	/* this call has to be in a timer callback, otherwise it causes
	 * gaim to segfault. */
	gaim_conversation_destroy(c);

	return FALSE;
}

static GaimCmdRet
irssi_window_cb(GaimConversation *c, const gchar *cmd, gchar **args,
		gchar **error, void *data)
{ /* callback for /window and /win */
	GaimGtkConversation *gtkconv;
	GaimGtkWindow *win;
	gchar *lower;
	gint curconv;

	/* the gtkconv is the only way to do some useful stuff below */
	gtkconv = GAIM_GTK_CONVERSATION(c);
	win = gtkconv->win; /* we need the window to get the GtkNotebook */
	/* we need the current notebook page for several operations below */
	curconv = gtk_notebook_get_current_page(GTK_NOTEBOOK(win->notebook));

	/* if *args or args[0] are NULL, we have a problem, since Gaim should
	 * only call us when an argument has been given */
	if(!*args && !args[0]) {
		*error = g_strdup("Major problem here folks!");
		return GAIM_CMD_RET_FAILED;
	}

	/* if the argument is a number, or starts with one, assume the user wants
	 * to switch to a specific numbered tab */
	if(g_ascii_isdigit(*args[0])) {
		gint temp;

		temp = atoi(args[0]) - 1; /* tab numbering starts at 0 */

		/* this actually changes the tab, but only if the number given points
		 * to a valid tab.  Stolen from Gaim's alt+n code */
		if(temp < gaim_gtk_conv_window_get_gtkconv_count(win))
			gtk_notebook_set_current_page(GTK_NOTEBOOK(win->notebook), temp);

		return GAIM_CMD_RET_OK;
	}

	/* we want this in lowercase in case the user used caps somewhere.
	 * I could probably have used strcasecmp, but I copied this part from
	 * the XMMS Remote plugin */
	lower = g_ascii_strdown(args[0], strlen(args[0]));

	if(!strcmp(lower, "close")) { /* close the conversation */
		g_timeout_add(50, (GSourceFunc)irssi_window_close_cb, c);
		return GAIM_CMD_RET_OK;
	/* the GtkNotebook stuff below was shamelessly stolen from Gaim's
	 * Ctrl+PageDown code */
	} else if(!strcmp(lower, "next") || !strcmp(lower, "right")) {
		/* if we're in the last tab, we need to go back to the first */
		if (!gaim_gtk_conv_window_get_gtkconv_at_index(win, curconv + 1))
			gtk_notebook_set_current_page(GTK_NOTEBOOK(win->notebook), 0);
		else /* otherwise just move to the next tab */
			gtk_notebook_set_current_page(GTK_NOTEBOOK(win->notebook), curconv + 1);

		return GAIM_CMD_RET_OK;
	/* the GtkNotebook stuff below was shamelessly stolen from Gaim's
	 * Ctrl+PageUp code */
	} else if(!strcmp(lower, "previous") || !strcmp(lower, "prev") || !strcmp(lower, "left")) {
		/* if we're in the first tab, we need to go back to the last */
		if (!gaim_gtk_conv_window_get_gtkconv_at_index(win, curconv - 1))
			gtk_notebook_set_current_page(GTK_NOTEBOOK(win->notebook), -1);
		else /* otherwise we just move to the previous tab */
			gtk_notebook_set_current_page(GTK_NOTEBOOK(win->notebook), curconv - 1);

		return GAIM_CMD_RET_OK;
	} else { /* here the user didn't enter a valid argument */
		*error = g_strdup(_("Invalid argument"));
		return GAIM_CMD_RET_FAILED;
	}

	/* if we reach here there are serious execution path problems... */
	return GAIM_CMD_RET_FAILED;
}

static GaimCmdRet
irssi_layout_cb(GaimConversation *c, const gchar *cmd, gchar **args,
		gchar **error, void *data)
{ /* callback for /layout */
	/* we're not doing anything here yet */
	return GAIM_CMD_RET_FAILED;
}

static GaimCmdRet
irssi_lastlog_cb(GaimConversation *c, const gchar *cmd, gchar **args,
		gchar **error, void *data)
{ /* callback for /lastlog */
	GaimGtkConversation *gtkconv = c->ui_data;
	int i;
	GString *result;
	char **lines;

	/* let's avoid some warnings on anal C compilers like mipspro cc */
	result = g_string_new(NULL);
	lines = gtk_imhtml_get_markup_lines(GTK_IMHTML(gtkconv->imhtml));

		/* XXX: This will include all messages, including the output of the
		 * history plugin, system messages, timestamps etc. This might be
		 * undesirable. A better solution will probably be considerably more
		 * complex.
		 */

	for (i = 0; lines[i]; i++) {
		char *strip = gaim_markup_strip_html(lines[i]);
		if (strstr(strip, args[0])) {
			result = g_string_append(result, lines[i]);
			result = g_string_append(result, "<br>");
		}
		g_free(strip);
	}

	gaim_notify_formatted(gtkconv, _("Lastlog"), _("Lastlog output"), NULL, result->str, NULL, NULL);

	g_string_free(result, TRUE);
	g_free(lines);
	return GAIM_CMD_RET_OK;
}

/* these three callbacks are intended to modify text so that formatting appears
 * similarly to how irssi would format the text */
/* TODO: make these callbacks actually do something! */
static gboolean
irssi_writing_cb(GaimAccount *account, const char *who, char **message,
		GaimConversation *conv, GaimMessageFlags flags)
{
	/* TODO: here we need to make sure we don't try to double format anything.
	 *       I'm open to any suggestions. */
	return FALSE;
}

static gboolean
irssi_sending_im_cb(GaimAccount *account, const char *receiver, char **message)
{
	return FALSE;
}

static gboolean
irssi_sending_chat_cb(GaimAccount *account, char **message, int id)
{
	return FALSE;
}

static gboolean
irssi_load(GaimPlugin *plugin) { /* Gaim calls this to load the plugin */
	const gchar *window_help, *win_help, *layout_help, *lastlog_help;
	void *convhandle;
	time_t t;
	
	convhandle = gaim_conversations_get_handle();
	t = time(NULL);
	lastday = irssi_datechange_get_day(&t);

	/*
	 * XXX: Translators: DO NOT TRANSLATE the first occurance of the word
	 * "window" below, or "close", "next", "previous", "left", or "right"
	 * at the *beginning* of the lines below!  The options to /window are
	 * NOT going to be translatable.  Also, please don't translate the HTML
	 * tags.
	 */
	window_help = _("<pre>window &lt;option&gt;: Operations for windows "
					"(tabs).  Valid options are:\n"
					"close - closes the current conversation\n"
					"next - move to the next conversation\n"
					"previous - move to the previous conversation\n"
					"left - move one conversation to the left\n"
					"right - move one conversation to the right\n"
					"&lt;number&gt; - go to tab <number>\n</pre>");

	/* 
	 * XXX: Translators: DO NOT TRANSLATE "win", "/window", "/help window",
	 * or the HTML in the lines below!
	 */
	win_help = _("<pre>win: This command is synonymous with /window.  Try "
				"/help window for further details.</pre>");

	/*
	 * XXX: Translators: DO NOT TRANSLATE the first "layout" or the "\nsave"
	 * or "reset" at the beginning of the last line below, or the HTML tags.
	 */
	layout_help = _("<pre>layout &lt;save|reset&gt;: Remember the layout of "
					"the current conversations to reopen them when Gaim is "
					"restarted.\nsave - saves the current layout\n"
					"reset - clears the current saved layout\n</pre>");

	/* XXX: Translators: DO NOT TRANSLATE "lastlog" or the HTML tags below */
	lastlog_help = _("<pre>lastlog &lt;string&gt;: Shows, from the current "
				"conversation's history, all messages containing the word or "
				"words specified in string.  It will be an exact match, "
				"including whitespace and special characters.");

	/* every 60 seconds we want to check to see if the day has changed so
	 * we can print the day changed notice if it has */
	irssi_datechange_timeout_id = gaim_timeout_add(60000,
						irssi_datechange_timeout_cb, NULL);

	/*
	 * registering the /window command so that it will take only one argument,
	 * matched as a single word, mark it as a plugin-priority command, make it
	 * functional in both IMs and chats, set it such that it is NOT
	 * protocol-specific, specify the callback and the help string, and set
	 * no user data to pass to the callback.
	 */
	irssi_window_cmd = gaim_cmd_register("window", "w", GAIM_CMD_P_PLUGIN,
			GAIM_CMD_FLAG_IM | GAIM_CMD_FLAG_CHAT, NULL,
			GAIM_CMD_FUNC(irssi_window_cb), window_help, NULL);

	/* same thing as above, except for the /win command */
	irssi_win_cmd = gaim_cmd_register("win", "w", GAIM_CMD_P_PLUGIN,
			GAIM_CMD_FLAG_IM | GAIM_CMD_FLAG_CHAT, NULL,
			GAIM_CMD_FUNC(irssi_window_cb), window_help, NULL);

	/*
	 * registering the /layout command so that it will take only one argument,
	 * matched as a single word, mark it as a plugin-priority command, make it
	 * functional in both IMs and chats, set it such that it is NOT
	 * protocol-specific, specify the callback and the help string, and set
	 * no user data to pass to the callback.
	 */
	irssi_layout_cmd = gaim_cmd_register("layout", "w", GAIM_CMD_P_PLUGIN,
			GAIM_CMD_FLAG_IM | GAIM_CMD_FLAG_CHAT, NULL,
			GAIM_CMD_FUNC(irssi_layout_cb), layout_help, NULL);

	/*
	 * registering the /lastlog command so that it will take an arbitrary
	 * number of arguments 1 or greater, matched as a single string, mark it
	 * as a plugin-priority command, make it functional in both IMs and chats,
	 * set it such that it is NOT protocol-specific, specify the callback and
	 * the help string, and set no user data to pass to the callback
	 */
	irssi_lastlog_cmd = gaim_cmd_register("lastlog", "s", GAIM_CMD_P_PLUGIN,
			GAIM_CMD_FLAG_IM | GAIM_CMD_FLAG_CHAT, NULL,
			GAIM_CMD_FUNC(irssi_lastlog_cb), lastlog_help, NULL);

	/* here we connect to the {writing,sending}-{chat,im}-msg signals so
	 * we can modify message text for stuff like *text*, /text/, and _text_ */
	gaim_signal_connect(convhandle, "writing-im-msg", plugin,
			GAIM_CALLBACK(irssi_writing_cb), NULL);
	gaim_signal_connect(convhandle, "writing-chat-msg", plugin,
			GAIM_CALLBACK(irssi_writing_cb), NULL);
	gaim_signal_connect(convhandle, "sending-im-msg", plugin,
			GAIM_CALLBACK(irssi_sending_im_cb), NULL);
	gaim_signal_connect(convhandle, "sending-chat-msg", plugin,
			GAIM_CALLBACK(irssi_sending_chat_cb), NULL);

	return TRUE; /* continue loading the plugin */
}

static gboolean
irssi_unload(GaimPlugin *plugin) { /* Gaim calls this to unload the plugin */
	/* unset this timeout handler because Gaim would segfault if this plugin
	 * were unloaded but the timeout left in place before Gaim closed. */
	gaim_timeout_remove(irssi_datechange_timeout_id);

	/* unregister these commands so that if the plugin is unloaded before Gaim
	 * is closed, Gaim will not segfault if the commands are used. */
	gaim_cmd_unregister(irssi_window_cmd);
	gaim_cmd_unregister(irssi_win_cmd);
	gaim_cmd_unregister(irssi_layout_cmd);
	gaim_cmd_unregister(irssi_lastlog_cmd);

	return TRUE; /* continue unloading the plugin */
}

static GaimPluginInfo irssi_info = { /* this tells Gaim about the plugin */
	GAIM_PLUGIN_MAGIC,			/* Magic				*/
	GAIM_MAJOR_VERSION,			/* Gaim Major Version	*/
	GAIM_MINOR_VERSION,			/* Gaim Minor Version	*/
	GAIM_PLUGIN_STANDARD,		/* plugin type			*/
	GAIM_GTK_PLUGIN_TYPE,		/* ui requirement		*/
	0,							/* flags				*/
	NULL,						/* dependencies			*/
	GAIM_PRIORITY_DEFAULT,		/* priority				*/

	PLUGIN_ID,					/* plugin id			*/
	NULL,						/* name					*/
	GPP_VERSION,				/* version				*/
	NULL,						/* summary				*/
	NULL,						/* description			*/
	PLUGIN_AUTHOR,				/* author				*/
	GPP_WEBSITE,				/* website				*/

	irssi_load,					/* load					*/
	irssi_unload,				/* unload				*/
	NULL,						/* destroy				*/

	NULL,						/* ui_info				*/
	NULL,						/* extra_info			*/
	NULL,						/* prefs_info			*/
	NULL						/* actions				*/
};

static void
irssi_init(GaimPlugin *plugin) { /* Gaim calls this to initialize the plugin */

/* if the user hasn't disabled internationalization support, tell gettext
 * what package we're from and where our translations are, then set gettext
 * to use UTF-8 */
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	/* set these here to allow for translations of the strings */
	irssi_info.name = _(PLUGIN_NAME);
	irssi_info.summary = _(PLUGIN_SUMMARY);
	irssi_info.description = _(PLUGIN_DESCRIPTION);
}

GAIM_INIT_PLUGIN(PLUGIN_STATIC_NAME, irssi_init, irssi_info)


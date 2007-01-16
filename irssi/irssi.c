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
#ifdef HAVE_CONFIG_H
# include "../gpp_config.h"
#endif

#ifdef HAVE_REGEX_H
# include <regex.h>
#endif /* HAVE_REGEX_H */

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

/* Gaim headers */
#include <gtkplugin.h>
#include <plugin.h>
#include <version.h>

/* irssi headers */
#include "datechange.h"
#include "lastlog.h"
#include "layout.h"
#include "window.h"

/* Pack/Local headers */
#include "../common/i18n.h"

# if 0

#ifdef HAVE_REGEX_H
static gchar *
irssi_textfmt_regex_helper(gchar *message, const gchar *token,
						   const gchar *tag)
{
	GString *str = NULL;
	gchar *ret = NULL, *expr = NULL, *iter = message;
	regex_t regex;
	regmatch_t matches[4]; /* adjust if necessary */
	size_t nmatches = sizeof(matches) / sizeof(matches[0]);

	/* build our expression */
	expr = g_strdup_printf("[^ \t]%s.[$ \t]", token);

	/* compile the expression.
	 * We may want to move this so we only do it once for each type, but this
	 * is pretty cheap, resource-wise.
	 */
	if(regcomp(&regex, expr, REG_EXTENDED) != 0) {
		g_free(expr);
		return message;
	}

	/* our regex compiled fine, clean up our allocated expression */
	g_free(expr);

	/* now do our checking */
	if(regexec(&regex, iter, nmatches, matches, 0) == 0) {
		do {
			size_t m;
			gint offset = 0;

			for(m = 0; m < nmatches; m++) {
				if(matches[m].rm_eo == -1)
					break;
			}

			iter += offset;
		} while(regexec(&regex, iter, nmatches, matches, REG_NOTBOL) == 0);
	}

	ret = str->str;
	g_string_free(str, FALSE);

	return ret;
}

static gchar *
irssi_textfmt_regex(gchar *message) {
	message = irssi_textfmt_regex_helper(message, "*", "b");
	message = irssi_textfmt_regex_helper(message, "_", "u");
	message = irssi_textfmt_regex_helper(message, "/", "i");
	message = irssi_textfmt_regex_helper(message, "-", "s");

	return message;
}
#endif /* HAVE_REGEX_H */



/* these three callbacks are intended to modify text so that formatting appears
 * similarly to how irssi would format the text */
static gboolean
irssi_writing_cb(GaimAccount *account, const char *who, char **message,
				 GaimConversation *conv, GaimMessageFlags flags)
{
#ifdef HAVE_REGEX_H
	if(!message)
		return FALSE;

	*message = irssi_textfmt_regex(*message);
#endif /* HAVE_REGEX_H */

	return FALSE;
}

static gboolean
irssi_sending_im_cb(GaimAccount *account, const char *receiver, char **msg) {
#ifdef HAVE_REGEX_H
	if(!msg)
		return FALSE;

	*msg = irssi_textfmt_regex(*msg);
#endif /* HAVE_REGEX_H */

	return FALSE;
}

static gboolean
irssi_sending_chat_cb(GaimAccount *account, char **message, int id) {
#ifdef HAVE_REGEX_H
	if(!message)
		return FALSE;

	*message = irssi_textfmt_regex(*message);
#endif /* HAVE_REGEX_H */

	return FALSE;
}

#endif /* #if 0 */

static gboolean
irssi_load(GaimPlugin *plugin) { /* Gaim calls this to load the plugin */
#if 0
	const gchar *layout_help;
	void *convhandle;
	
	convhandle = gaim_conversations_get_handle();

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
#endif

	irssi_datechange_init(plugin);
	irssi_lastlog_init(plugin);
	irssi_layout_init(plugin);
	irssi_window_init(plugin);

	return TRUE;
}

static gboolean
irssi_unload(GaimPlugin *plugin) {
	irssi_window_uninit(plugin);
	irssi_layout_uninit(plugin);
	irssi_lastlog_uninit(plugin);
	irssi_datechange_uninit(plugin);

	return TRUE;
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


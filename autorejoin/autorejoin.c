/*
 * Autorejoin - Autorejoin on /kick
 * Copyright (C) 2006
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

#include "../common/pp_internal.h"

#define PLUGIN_ID			"core-plugin_pack-autorejoin"
#define PLUGIN_NAME			"Autorejoin (IRC)"
#define PLUGIN_STATIC_NAME	"Autorejoin (IRC)"
#define PLUGIN_SUMMARY		"Autorejoin on /kick on IRC"
#define PLUGIN_DESCRIPTION	"Automatically rejoin a IRC chatroom when someone lovingly /kicks you."
#define PLUGIN_AUTHOR		"Sadrul H Chowdhury <sadrul@users.sourceforge.net>"

/* System headers */
#include <string.h>

/* Purple headers */
#include <cmds.h>
#include <plugin.h>
#include <prpl.h>

static gboolean
show_them(gpointer data)
{
	/* So you think you can kick me? I'll show you! */
	PurpleConversation *conv = data;
	char *command = g_strdup_printf("join %s", purple_conversation_get_name(conv));
	char *markup = g_markup_escape_text(command, -1);
	char *error = NULL;
	purple_cmd_do_command(conv, command, markup, &error);  /* Do anything with the return value? */
	g_free(command);
	g_free(markup);
	g_free(error);
	return FALSE;
}

static void
irc_receiving_text(PurpleConnection *gc, const char **incoming, gpointer null)
{
	char **splits;

	if (!incoming || !*incoming || !**incoming)   /* oh the fun .. I can do this all day! */
		return;

	splits = g_strsplit(*incoming, " ", -1);
	if (splits[1]) {
		PurpleAccount *account = purple_connection_get_account(gc);
		char *str = g_ascii_strdown(splits[1], -1);

		if (strcmp(str, "kick") == 0 && splits[2] && splits[3]) {
			char *name = splits[2];
			GList *chats = purple_get_chats();
			while (chats) {
				PurpleConversation *conv = chats->data;
				chats = chats->next;
				if (purple_conversation_get_account(conv) == account
						&& strcmp(purple_conversation_get_name(conv), name) == 0) {
					g_timeout_add(1000, show_them, conv);
					break;
				}
			}
		}
		g_free(str);
	}
	g_strfreev(splits);
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	PurplePlugin *prpl = purple_find_prpl("prpl-irc");
	if (!prpl)
		return FALSE;
	purple_signal_connect(prpl, "irc-receiving-text", plugin,
				G_CALLBACK(irc_receiving_text), NULL);
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	return TRUE;
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,			/* Magic				*/
	PURPLE_MAJOR_VERSION,			/* Purple Major Version	*/
	PURPLE_MINOR_VERSION,			/* Purple Minor Version	*/
	PURPLE_PLUGIN_STANDARD,			/* plugin type			*/
	NULL,							/* ui requirement		*/
	0,								/* flags				*/
	NULL,							/* dependencies			*/
	PURPLE_PRIORITY_DEFAULT,		/* priority				*/

	PLUGIN_ID,						/* plugin id			*/
	NULL,							/* name					*/
	PP_VERSION,						/* version				*/
	NULL,							/* summary				*/
	NULL,							/* description			*/
	PLUGIN_AUTHOR,					/* author				*/
	PP_WEBSITE,						/* website				*/

	plugin_load,					/* load					*/
	plugin_unload,					/* unload				*/
	NULL,							/* destroy				*/

	NULL,							/* ui_info				*/
	NULL,							/* extra_info			*/
	NULL,							/* prefs_info			*/
	NULL,							/* actions				*/
	NULL,							/* reserved 1			*/
	NULL,							/* reserved 2			*/
	NULL,							/* reserved 3			*/
	NULL							/* reserved 4			*/
};

static void
init_plugin(PurplePlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _(PLUGIN_NAME);
	info.summary = _(PLUGIN_SUMMARY);
	info.description = _(PLUGIN_DESCRIPTION);
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info)

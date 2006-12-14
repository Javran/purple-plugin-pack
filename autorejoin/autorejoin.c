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
#ifdef HAVE_CONFIG_H
# include "../gpp_config.h"
#endif

#define GAIM_PLUGINS

#define PLUGIN_ID			"core-plugin_pack-autorejoin"
#define PLUGIN_NAME			"Autorejoin (IRC)"
#define PLUGIN_STATIC_NAME	"Autorejoin (IRC)"
#define PLUGIN_SUMMARY		"Autorejoin on /kick on IRC"
#define PLUGIN_DESCRIPTION	"Automatically rejoin a IRC chatroom when someone lovingly /kicks you."
#define PLUGIN_AUTHOR		"Sadrul H Chowdhury <sadrul@users.sourceforge.net>"

/* System headers */
#include <glib.h>
#include <string.h>

/* Gaim headers */
#include <cmds.h>
#include <plugin.h>
#include <prpl.h>
#include <version.h>

/* Pack/Local headers */
#include "../common/i18n.h"

static gboolean
show_them(gpointer data)
{
	/* So you think you can kick me? I'll show you! */
	GaimConversation *conv = data;
	char *command = g_strdup_printf("join %s", gaim_conversation_get_name(conv));
	char *markup = g_markup_escape_text(command, -1);
	char *error = NULL;
	gaim_cmd_do_command(conv, command, markup, &error);  /* Do anything with the return value? */
	g_free(command);
	g_free(markup);
	g_free(error);
	return FALSE;
}

static void
irc_receiving_text(GaimConnection *gc, const char **incoming, gpointer null)
{
	char **splits;

	if (!incoming || !*incoming || !**incoming)   /* oh the fun .. I can do this all day! */
		return;

	splits = g_strsplit(*incoming, " ", -1);
	if (splits[1]) {
		GaimAccount *account = gaim_connection_get_account(gc);
		char *str = g_ascii_strdown(splits[1], -1);

		if (strcmp(str, "kick") == 0 && splits[2] && splits[3]) {
			char *name = splits[2];
			GList *chats = gaim_get_chats();
			while (chats) {
				GaimConversation *conv = chats->data;
				chats = chats->next;
				if (gaim_conversation_get_account(conv) == account
						&& strcmp(gaim_conversation_get_name(conv), name) == 0) {
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
plugin_load(GaimPlugin *plugin)
{
	GaimPlugin *prpl = gaim_find_prpl("prpl-irc");
	if (!prpl)
		return FALSE;
	gaim_signal_connect(prpl, "irc-receiving-text", plugin,
				G_CALLBACK(irc_receiving_text), NULL);
	return TRUE;
}

static gboolean
plugin_unload(GaimPlugin *plugin)
{
	return TRUE;
}

static GaimPluginInfo info = {
	GAIM_PLUGIN_MAGIC,			/* Magic				*/
	GAIM_MAJOR_VERSION,			/* Gaim Major Version	*/
	GAIM_MINOR_VERSION,			/* Gaim Minor Version	*/
	GAIM_PLUGIN_STANDARD,		/* plugin type			*/
	NULL,						/* ui requirement		*/
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

	plugin_load,				/* load					*/
	plugin_unload,				/* unload				*/
	NULL,						/* destroy				*/

	NULL,						/* ui_info				*/
	NULL,						/* extra_info			*/
	NULL,						/* prefs_info			*/
	NULL						/* actions				*/
};

static void
init_plugin(GaimPlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _(PLUGIN_NAME);
	info.summary = _(PLUGIN_SUMMARY);
	info.description = _(PLUGIN_DESCRIPTION);
}

GAIM_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info)

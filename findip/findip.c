/*
 * Find IP - Find the IP of a person in the buddylist
 * Copyright (C) 2007
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02111-1301, USA.
 */

/* If you can't figure out what this line is for, DON'T TOUCH IT. */
#include "../common/pp_internal.h"

#define PLUGIN_ID			"core-plugin_pack-findip"
#define PLUGIN_STATIC_NAME	"findip"
#define PLUGIN_AUTHOR		"someone <someone@somewhere.tld>"

/* System headers */
#include <glib.h>

/* Purple headers */
#include <plugin.h>
#include <blist.h>
#include <eventloop.h>
#include <server.h>
#include <version.h>

/* If you can't figure out what this line is for, DON'T TOUCH IT. */
#include "../common/pp_internal.h"

#define PREF_ROOT "/plugins/core/plugin_pack/" PLUGIN_STATIC_NAME
#define PREF_NOTIFY PREF_ROOT "/notify"

static gboolean
show_ip(gpointer node)
{
	PurpleBuddy *buddy;
	PurpleConversation *conv;

	buddy = (PurpleBuddy*)node;
	if (buddy->account == NULL || buddy->account->gc == NULL)
		return FALSE;

	conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, buddy->account, buddy->name);
	purple_conversation_write(conv, NULL, _("Looked up IP: 127.0.0.1\n"),
			PURPLE_MESSAGE_NOTIFY | PURPLE_MESSAGE_SYSTEM,
			time(NULL));
	if (purple_prefs_get_bool(PREF_NOTIFY))
		serv_send_im(buddy->account->gc, buddy->name, _("Yo! What's your IP?"), 0);
	return FALSE;
}

static void
find_ip(PurpleBlistNode *node, gpointer plugin)
{
	PurpleConversation *conv;
	PurpleBuddy *buddy;

	if (PURPLE_BLIST_NODE_IS_CONTACT(node))
		node = (PurpleBlistNode*)purple_contact_get_priority_buddy((PurpleContact*)node);
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node))
		return;

	purple_timeout_add_seconds(5, show_ip, node);

	buddy = (PurpleBuddy*)node;
	conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, buddy->account, buddy->name);
	purple_conversation_write(conv, NULL, _("Looking up the IP ...\n"),
			PURPLE_MESSAGE_NOTIFY | PURPLE_MESSAGE_SYSTEM,
			time(NULL));
}

static void
context_menu(PurpleBlistNode *node, GList **menu, gpointer plugin)
{
	PurpleMenuAction *action;

	if (!PURPLE_BLIST_NODE_IS_BUDDY(node) && !PURPLE_BLIST_NODE_IS_CONTACT(node))
		return;

	action = purple_menu_action_new(_("Find IP"),
					PURPLE_CALLBACK(find_ip), plugin, NULL);
	(*menu) = g_list_prepend(*menu, action);
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	purple_signal_connect(purple_blist_get_handle(), "blist-node-extended-menu", plugin,
						PURPLE_CALLBACK(context_menu), plugin);
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	return TRUE;
}

static PurplePluginPrefFrame *
get_plugin_pref_frame(PurplePlugin *plugin)
{
	PurplePluginPrefFrame *frame;
	PurplePluginPref *pref;

	frame = purple_plugin_pref_frame_new();

	pref = purple_plugin_pref_new_with_name_and_label(PREF_NOTIFY,
			_("Notify the user that you are trying to get the IP"));
	purple_plugin_pref_frame_add(frame, pref);

	return frame;
}

static PurplePluginUiInfo pref_info = {
	get_plugin_pref_frame,
	0,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,		/* Magic				*/
	PURPLE_MAJOR_VERSION,		/* Purple Major Version	*/
	PURPLE_MINOR_VERSION,		/* Purple Minor Version	*/
	PURPLE_PLUGIN_STANDARD,		/* plugin type			*/
	NULL,						/* ui requirement		*/
	0,							/* flags				*/
	NULL,						/* dependencies			*/
	PURPLE_PRIORITY_DEFAULT,	/* priority				*/

	PLUGIN_ID,					/* plugin id			*/
	NULL,						/* name					*/
	PP_VERSION,					/* version				*/
	NULL,						/* summary				*/
	NULL,						/* description			*/
	PLUGIN_AUTHOR,				/* author				*/
	PP_WEBSITE	,				/* website				*/

	plugin_load,				/* load					*/
	plugin_unload,				/* unload				*/
	NULL,						/* destroy				*/

	NULL,						/* ui_info				*/
	NULL,						/* extra_info			*/
	&pref_info,					/* prefs_info			*/
	NULL						/* actions				*/
};

static void
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Find IP");
	info.summary = _("Find the IP of a person in the buddylist.");
	info.description = _("Find the IP of a person in the buddylist. This doesn't really work.");

	purple_prefs_add_none(PREF_ROOT);
	purple_prefs_add_bool(PREF_NOTIFY, TRUE);
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info)

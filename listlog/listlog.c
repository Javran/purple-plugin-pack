/*
 * Listlog - Logs user list on chat join
 * Copyright (C) 2008 John Bailey <rekkanoryo@rekkanoryo.org>
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

#include "../common/pp_internal.h"

#define PLUGIN_ID			"gtk-plugin_pack-listlog"
#define PLUGIN_STATIC_NAME	"listlog"
#define PLUGIN_AUTHOR		"John Bailey <rekkanoryo@rekkanoryo.org>"

/* libc headers */
#include <time.h>

/* Purple headers */
#include <conversation.h>
#include <debug.h>

/* Pidgin headers */
#include <gtkplugin.h>

static void
listlog_chat_joined_cb(PurpleConversation *conv)
{
	GList *users = NULL;
	GString *message = NULL;
	PurpleConvChat *chat = NULL;

	purple_debug_misc(PLUGIN_ID, "Conversation pointer: %p\n", conv);

	chat = purple_conversation_get_chat_data(conv);

	purple_debug_misc(PLUGIN_ID, "Chat pointer: %p\n", chat);

	users = purple_conv_chat_get_users(chat);

   purple_debug_misc(PLUGIN_ID, "List of users pointer: %p\n", users);

	message = g_string_new("List of users:\n");

	while(users) {
		g_string_append_printf(message, "[ %s ]", purple_conv_chat_cb_get_name(users->data));

		users = users->next;
	}

	purple_conversation_write(conv, NULL, message->str,
			PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_NOTIFY, time(NULL));
}

static gboolean
listlog_load(PurplePlugin *plugin)
{
	purple_signal_connect(purple_conversations_get_handle(), "chat-joined",
			plugin, PURPLE_CALLBACK(listlog_chat_joined_cb), NULL);

	return TRUE;
}

static gboolean
listlog_unload(PurplePlugin *plugin)
{
	return TRUE;
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,		/* Magic				*/
	PURPLE_MAJOR_VERSION,		/* Purple Major Version	*/
	PURPLE_MINOR_VERSION,		/* Purple Minor Version	*/
	PURPLE_PLUGIN_STANDARD,		/* plugin type			*/
	PIDGIN_PLUGIN_TYPE,			/* ui requirement		*/
	0,							/* flags				*/
	NULL,						/* dependencies			*/
	PURPLE_PRIORITY_DEFAULT,	/* priority				*/

	PLUGIN_ID,					/* plugin id			*/
	NULL,						/* name					*/
	PP_VERSION,					/* version				*/
	NULL,						/* summary				*/
	NULL,						/* description			*/
	PLUGIN_AUTHOR,				/* author				*/
	PP_WEBSITE,					/* website				*/

	listlog_load,				/* load					*/
	listlog_unload,				/* unload				*/
	NULL,						/* destroy				*/

	NULL,						/* ui_info				*/
	NULL,						/* extra_info			*/
	NULL,						/* prefs_info			*/
	NULL,						/* actions				*/

	NULL,						/* reserved 1			*/
	NULL,						/* reserved 2			*/
	NULL,						/* reserved 3			*/
	NULL						/* reserved 4			*/
};

static void
listlog_init(PurplePlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Chat User List Logging");
	info.summary = _("Logs the list of users present when you join a chat.");
	info.description = _("Logs the list of users present when you join a chat.");
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, listlog_init, info)


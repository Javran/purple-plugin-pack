/*
 * nomobility - stops you from sending messages to mobile users
 * Copyright (C) 2008 Gary Kramlich <grim@reaperworld.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301, USA.
 */

/* If you can't figure out what this line is for, DON'T TOUCH IT. */
#include "../common/pp_internal.h"

#include <time.h>

#include <plugin.h>

#include <blist.h>
#include <conversation.h>
#include <debug.h>
#include <status.h>

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
sending_im_msg(PurpleAccount *account, gchar *receiver, gchar **message,
               gpointer data)
{
	PurpleBuddy *buddy = NULL;
	PurplePresence *presence = NULL;

	buddy = purple_find_buddy(account, receiver);

	if(!buddy)
		return;

	presence = purple_buddy_get_presence(buddy);
	
	if(!presence)
		return;

#if 0
	if(purple_presence_is_status_primitive_active(presence,
	                                              PURPLE_STATUS_MOBILE))
#endif
	{
		PurpleConversation *conv = NULL;
		gchar *msg = NULL;

		purple_debug_info("nomobility", "killing message!\n");
		g_free(*message);
		*message = NULL;

		conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM,
		                                             receiver, account);
		if(!conv)
			return;

		msg = g_strdup_printf(_("Cancelled message to %s, they are currently "
		                        "mobile."),
								receiver);
		purple_conv_im_write(PURPLE_CONV_IM(conv), receiver, msg,
		                     PURPLE_MESSAGE_NO_LOG | PURPLE_MESSAGE_DELAYED,
							 time(NULL));
		g_free(msg);
	}
}

/******************************************************************************
 * Plugin Stuff
 *****************************************************************************/
static gboolean
plugin_load(PurplePlugin *plugin) {
	purple_signal_connect(purple_conversations_get_handle(),
	                      "sending-im-msg", plugin,
	                      PURPLE_CALLBACK(sending_im_msg), NULL);
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	return TRUE;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,	
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,

	"core-plugin_pack-nomobility",
	NULL,
	PP_VERSION,	
	NULL,
	NULL,
	"Gary Kramlich <grim@reaperworld.com>",	
	PP_WEBSITE,	

	plugin_load,
	plugin_unload,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("nomobility");
	info.summary = _("Stops you from messaging mobile users");
	info.description = info.summary;

}

PURPLE_INIT_PLUGIN(nomobility, init_plugin, info)

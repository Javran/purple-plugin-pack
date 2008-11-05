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
#include <cmds.h>
#include <conversation.h>
#include <debug.h>
#include <status.h>

/******************************************************************************
 * Globals
 *****************************************************************************/
#define NO_MOBILITY_QUEUE_KEY "no-mobility"
#define NO_MOBILITY_COMMAND   "mobile"

static PurpleCmdId nomobility_cmd_id = 0;

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
nomobility_list_messages(PurpleConversation *conv, GList *queue) {
	GList *l = NULL;
	const gchar *name = purple_conversation_get_name(conv);
	gint i = 0;

	if(!queue) {
		purple_conv_im_write(PURPLE_CONV_IM(conv), name,
		                     _("There are no messages in the queue."),
		                     PURPLE_MESSAGE_NO_LOG, time(NULL));

		return;
	}

	for(l = queue; l; l = l->next, i++) {
		gchar *msg = g_strdup_printf(_("%d. %s"), i + 1, (gchar *)l->data);

		purple_conv_im_write(PURPLE_CONV_IM(conv), name, msg,
		                     PURPLE_MESSAGE_NO_LOG, time(NULL));

		g_free(msg);
	}
}

static void
nomobility_clear(PurpleConversation *conv, GList *queue) {
	GList *l = NULL;

	for(l = queue; l; l = l->next)
		g_free(l->data);

	g_list_free(queue);

	purple_conversation_set_data(conv, NO_MOBILITY_QUEUE_KEY, NULL);
}

static void
nomobility_send(PurpleConversation *conv, GList *queue) {
	GList *l = NULL;
	GString *str = g_string_new("");

	for(l = queue; l; l = l->next) {
		gchar *msg = (gchar *)l->data;
		const gchar *suffix = (l->next) ? "\n" : "";

		g_string_append_printf(str, "%s%s", msg, suffix);
	}

	purple_conv_im_send(PURPLE_CONV_IM(conv), str->str);

	g_string_free(str, TRUE);

	nomobility_clear(conv, queue);
}

static void
nomobility_delete(PurpleConversation *conv, GList *queue, gint n_msg) {
	GList *l = NULL;
	gint i = 0;

	for(l = queue; i < n_msg - 1; i++, l = l->next);

	if(l)
		g_free(l->data);

	queue = g_list_remove(queue, l);

	purple_conversation_set_data(conv, NO_MOBILITY_QUEUE_KEY, queue);
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
sending_im_msg(PurpleAccount *account, gchar *receiver, gchar **message,
               gpointer data)
{
	PurpleBuddy *buddy = NULL;
	PurplePresence *presence = NULL;

	if(!message || !*message)
		return;

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

		msg = g_strdup_printf(_("Cancelled message to %s, they are currently "
		                        "mobile."),
								receiver);

		conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM,
		                                             receiver, account);

		/* if we have the account, add the message to our queue */
		if(conv) {
			GList *queue = NULL;
			
			queue = purple_conversation_get_data(conv, NO_MOBILITY_QUEUE_KEY);

			queue = g_list_append(queue, g_strdup(*message));
			purple_conversation_set_data(conv, NO_MOBILITY_QUEUE_KEY, queue);
		}

		/* now actually kill the message */
		g_free(*message);
		*message = NULL;

		/* if we failed to find the conv, write a debug message and bail */
		if(!conv) {
			purple_debug_info("nomobility", "%s\n", msg);
			g_free(msg);

			return;
		}

		/* we have a conv, so note that we queue the message in conv */
		purple_conv_im_write(PURPLE_CONV_IM(conv), receiver, msg,
		                     PURPLE_MESSAGE_NO_LOG | PURPLE_MESSAGE_DELAYED,
							 time(NULL));
		g_free(msg);
	}
}

static PurpleCmdRet
nomobility_cmd(PurpleConversation *conv, const gchar *cmd, gchar **args,
               gchar **error, gpointer data)
{
	GList *queue = NULL;
	gchar *lower = NULL;
	
	if(!*args && !args[0]) {
		*error = g_strdup("eek!");

		return PURPLE_CMD_RET_FAILED;
	}

	queue = purple_conversation_get_data(conv, NO_MOBILITY_QUEUE_KEY);

	lower = g_ascii_strdown(args[0], strlen(args[0]));

	if(strcmp(lower, "clear") == 0) {
		nomobility_clear(conv, queue);
	} else if(strcmp(lower, "delete") == 0) {
		gint n_msg = 0;

		if(!args[1]) {
			*error = g_strdup(_("Delete failed: no message number given!"));

			return PURPLE_CMD_RET_FAILED;
		}

		n_msg = atoi(args[1]);
		if(n_msg < 0 || n_msg >= g_list_length(queue)) {
			*error =
				g_strdup_printf(_("Delete failed: no messaged numbered %d!"),
				                n_msg);

			return PURPLE_CMD_RET_FAILED;
		}

		nomobility_delete(conv, queue, n_msg);
	} else if(strcmp(lower, "list") == 0) {
		nomobility_list_messages(conv, queue);
	} else if(strcmp(lower, "sendall") == 0) {
		nomobility_send(conv, queue);
	}

	g_free(lower);

	return PURPLE_CMD_RET_OK;
}

/******************************************************************************
 * Plugin Stuff
 *****************************************************************************/
static gboolean
plugin_load(PurplePlugin *plugin) {
	PurpleCmdFlag flags = PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS;
	void *conv_handle = purple_conversations_get_handle();
	gchar *help = NULL;

	/* signals */
	purple_signal_connect(conv_handle, "sending-im-msg", plugin,
	                      PURPLE_CALLBACK(sending_im_msg), NULL);

	/* commands */
	help = g_strdup_printf(_("%s &lt;[clear][clear][delete][send]&gt;\n"
							 "clear     Clears all queued messages\n"
	                         "delete #  Deletes the message numbered #\n"
							 "list      Lists all queued messages\n"
							 "sendall   Sends all queued messages\n"),
	                         NO_MOBILITY_COMMAND);

	nomobility_cmd_id =
		purple_cmd_register(NO_MOBILITY_COMMAND, "ws", PURPLE_CMD_P_PLUGIN,
		                    flags, NULL,
		                    PURPLE_CMD_FUNC(nomobility_cmd), help, NULL);
	g_free(help);

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

	info.name = _("No Mobility");
	info.summary = _("Stops you from messaging mobile users");
	info.description = info.summary;

}

PURPLE_INIT_PLUGIN(nomobility, init_plugin, info)

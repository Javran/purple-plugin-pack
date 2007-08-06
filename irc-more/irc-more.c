/**
 * @file irc-more.c A couple of additional IRC features.
 *
 * Copyright (C) 2007 Sadrul Habib Chowdhury <sadrul@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
# include "../pp_config.h"
#endif

#define PURPLE_PLUGINS

#include <cmds.h>
#include <plugin.h>
#include <accountopt.h>
#include <conversation.h>
#include <version.h>

#include <string.h>

/* Pack/Local headers */
#include "../common/pp_internal.h"

#define CTCP_REPLY    purple_account_get_string(account, "ctcp-message", "Purple IRC")
#define PART_MESSAGE  purple_account_get_string(account, "part-message", "Leaving.")
#define QUIT_MESSAGE  purple_account_get_string(account, "quit-message", "Leaving.")

#define MATCHES(string)   !strncmp(*msg, string, sizeof(string) - 1)

static PurpleCmdId notice_cmd_id = 0;
static PurplePluginProtocolInfo *irc_info = NULL;

static PurpleCmdRet
notice_cmd_cb(PurpleConversation *conv, const gchar *cmd, gchar **args,
		gchar **error, void *data)
{
	gchar *tmp = NULL, *msg = NULL;
	gint len = 0, arg0len = 0, arg1len = 0, maxlen = 0;
	PurpleConnection *gc = NULL;

	if(!args && !args[0] && !args[1])
		return PURPLE_CMD_RET_FAILED;

	gc = purple_conversation_get_gc(conv);

	/* convenience to make the next comparison make more sense */
	arg0len = strlen(args[0]);
	arg1len = strlen(args[1]);

	/* IRC messages are limited to 512 bytes.  2 are reserved for CRLF, 2 for
	 * the spaces needed after the command and target, 1 for the colon needed
	 * before the notice text starts, and 6 for NOTICE.  Result is the length
	 * of the message needs to be limited to 501 bytes.  We need to account
	 * for the length of the nick or channel name that is the target, too. */
	maxlen = 501 - arg0len;

	if(arg1len > maxlen)
		tmp = g_strndup(args[1], maxlen);

	/* if tmp is not NULL, the notice the user wants to send is too long so we
	 * truncated it.  If tmp is NULL the notice is fine as-is.  Either way,
	 * assign msg as appropriate. */
	if(tmp)
		msg = tmp;
	else
		msg = args[1];
	
	msg = g_strdup_printf("NOTICE %s :%s\r\n", args[0], msg);

	len = strlen(msg);

	irc_info->send_raw(gc, msg, len);

	g_free(msg);
	g_free(tmp);

	return PURPLE_CMD_RET_OK;
}

static void
irc_sending_text(PurpleConnection *gc, char **msg, gpointer null)
{
	PurpleAccount *account = purple_connection_get_account(gc);
	char **old = msg;

	if (MATCHES("QUIT ")) {
		char *message = strchr(*msg, ':');
		if (!message || !strcmp(message + 1, "Leaving.\r\n")) {
			*msg = g_strdup_printf("QUIT :%s\r\n", QUIT_MESSAGE);
		}
	} else if (MATCHES("PART ")) {
		char *message = strchr(*msg, ':');
		if (message)
			return;   /* The user did give some part message. Do not use the default one. */
		message = strchr(*msg, '\r');
		*message = '\0';
		*msg = g_strdup_printf("%s :%s\r\n", *msg, PART_MESSAGE);
	} else if (MATCHES("NOTICE ")) {
		char *version = strstr(*msg, ":\001VERSION ");
		if (!version)
			return;
		*version = '\0';
		*msg = g_strdup_printf("%s:\001VERSION %s\001\r\n", *msg, CTCP_REPLY);
	}
	if (msg != old)
		g_free(*old);
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	PurplePlugin *prpl = purple_find_prpl("prpl-irc");
	PurpleAccountOption *option;
	gchar *notice_help = NULL;

	notice_help = _("notice target message:  Send a notice to the specified target.");

	notice_cmd_id = purple_cmd_register("notice", "ws", PURPLE_CMD_P_PLUGIN,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PRPL_ONLY,
			"prpl-irc", notice_cmd_cb, notice_help, NULL);

	if (!prpl)
		return FALSE;
	purple_signal_connect(prpl, "irc-sending-text", plugin,
				G_CALLBACK(irc_sending_text), NULL);

	irc_info = PURPLE_PLUGIN_PROTOCOL_INFO(prpl);

	option = purple_account_option_string_new(_("Default Quit Message"), "quit-message", "Leaving.");
	irc_info->protocol_options = g_list_append(irc_info->protocol_options, option);

	option = purple_account_option_string_new(_("Default Part Message"), "part-message", "Leaving.");
	irc_info->protocol_options = g_list_append(irc_info->protocol_options, option);

	option = purple_account_option_string_new(_("CTCP Version reply"), "ctcp-message", "Purple IRC");
	irc_info->protocol_options = g_list_append(irc_info->protocol_options, option);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	purple_cmd_unregister(notice_cmd_id);

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
	"irc-more",
	NULL,
	PP_VERSION,
	NULL,
	NULL,
	"Sadrul H Chowdhury <sadrul@users.sourceforge.net>",
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

	info.name = _("IRC More");
	info.summary = _("Adds additional IRC features.");
	info.description = _("Adds additional IRC features, including a "
			"customizable quit message, a customizable CTCP VERSION reply, "
			"and the /notice command for notices.");
}

PURPLE_INIT_PLUGIN(irc_more, init_plugin, info)


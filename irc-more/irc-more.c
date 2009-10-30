/**
 * @file irc-more.c A couple of additional IRC features.
 *
 * Copyright (C) 2007-2008 Sadrul Habib Chowdhury <sadrul@users.sourceforge.net>
 * Copyright (C) 2007-2008 John Bailey <rekkanoryo@rekkanoryo.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

/* If you can't figure out what this line is for, DON'T TOUCH IT. */
#include "../common/pp_internal.h"

#include <accountopt.h>
#include <cmds.h>
#include <conversation.h>
#include <plugin.h>
#include <prpl.h>

#include <string.h>

#define CTCP_REPLY    purple_account_get_string(account, "ctcp-message", "Purple IRC")
#define PART_MESSAGE  purple_account_get_string(account, "part-message", "Leaving.")
#define QUIT_MESSAGE  purple_account_get_string(account, "quit-message", "Leaving.")
#define SET_UMODES    purple_account_get_string(account, "setumodes", "i")
#define UNSET_UMODES  purple_account_get_string(account, "unsetumodes", NULL)

#define PLUGIN_ID "core-plugin_pack-irc-more"

#define PREF_PREFIX "/plugins/core/" PLUGIN_ID
#define PREF_DELAY PREF_PREFIX "/delay"

#define MATCHES(string)   !strncmp(*msg, string, sizeof(string) - 1)

static PurpleCmdId notice_cmd_id = 0;
static PurplePluginProtocolInfo *irc_info = NULL;

static gboolean
show_them(gpointer data)
{
	/* So you think you can kick me? I'll show you! */
	PurpleConversation *conv = data;
	char *conv_name = NULL, *command = NULL, *markup = NULL, *error = NULL;

	if(conv_name) {
		command = g_strdup_printf("join %s", conv_name);
		markup = g_markup_escape_text(command, -1);
		error = NULL;
		purple_cmd_do_command(conv, command, markup, &error);  /* Do anything with the return value? */
		g_free(command);
		g_free(markup);
		g_free(error);
	}

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
					g_timeout_add(1000 * MAX(10, purple_prefs_get_int(PREF_DELAY)), show_them, conv);
					break;
				}
			}
		}
		g_free(str);
	}
	g_strfreev(splits);
}

static void
signed_on_cb(PurpleConnection *gc)
{
	/* should this be done on a timeout? */
	PurpleAccount *account = NULL;
	const gchar *nick = NULL, *setmodes = NULL, *unsetmodes = NULL;
	gchar *msg = NULL, *msg2 = NULL;

	account = purple_connection_get_account(gc);

	/* hopefully prevent crashes related to non-IRC accounts signing on */
	if(strcmp("prpl-irc", purple_account_get_protocol_id(account)))
		return;

	nick = purple_connection_get_display_name(gc);
	setmodes = SET_UMODES;
	unsetmodes = UNSET_UMODES;

	msg = g_strdup_printf("MODE %s +%s\r\n", nick, setmodes);
	irc_info->send_raw(gc, msg, strlen(msg));
	g_free(msg);

	if(unsetmodes && *unsetmodes) {
		msg2 = g_strdup_printf("MODE %s -%s\r\n", nick, unsetmodes);
		irc_info->send_raw(gc, msg2, strlen(msg2));
		g_free(msg2);
	}

	return;
}

#if !PURPLE_VERSION_CHECK(2,4,0)
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

	/* avoid a possible double-free crash */
	if(msg != tmp)
		g_free(tmp);

	g_free(msg);

	return PURPLE_CMD_RET_OK;
}
#endif

static void
irc_sending_text(PurpleConnection *gc, char **msg, gpointer null)
{
	PurpleAccount *account = purple_connection_get_account(gc);
	char *old = *msg;

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
	if (*msg != old)
		g_free(old);
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	PurplePlugin *prpl = NULL;
	PurpleAccountOption *option;
	gchar *notice_help = NULL;
	void *gc_handle = NULL;

	prpl = purple_find_prpl("prpl-irc");

	/* if we didn't find the prpl, bail */
	if (!prpl)
		return FALSE;

	/* specify our help string and register our command */
	notice_help = _("notice target message:  Send a notice to the specified target.");

#if !PURPLE_VERSION_CHECK(2,4,0)
	notice_cmd_id = purple_cmd_register("notice", "ws", PURPLE_CMD_P_PLUGIN,
			PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_PRPL_ONLY,
			"prpl-irc", notice_cmd_cb, notice_help, NULL);
#endif

	/* we need this handle for the signed-on signal */
	gc_handle = purple_connections_get_handle();

	/* list signals in alphabetical order for consistency */
	purple_signal_connect(prpl, "irc-sending-text", plugin,
				G_CALLBACK(irc_sending_text), NULL);
	purple_signal_connect(prpl, "irc-receiving-text", plugin,
				G_CALLBACK(irc_receiving_text), NULL);
	purple_signal_connect(gc_handle, "signed-on", plugin,
				G_CALLBACK(signed_on_cb), NULL);

	irc_info = PURPLE_PLUGIN_PROTOCOL_INFO(prpl);

	/* Alphabetize the option label strings */
	option = purple_account_option_string_new(_("CTCP Version reply"), "ctcp-message", "Purple IRC");
	irc_info->protocol_options = g_list_append(irc_info->protocol_options, option);

	option = purple_account_option_string_new(_("Default Quit Message"), "quit-message", "Leaving.");
	irc_info->protocol_options = g_list_append(irc_info->protocol_options, option);

	option = purple_account_option_string_new(_("Default Part Message"), "part-message", "Leaving.");
	irc_info->protocol_options = g_list_append(irc_info->protocol_options, option);

	option = purple_account_option_string_new(_("Set User Modes On Connect"), "setumodes", "i");
	irc_info->protocol_options = g_list_append(irc_info->protocol_options, option);

	option = purple_account_option_string_new(_("Unset User Modes On Connect"), "unsetumodes", "");
	irc_info->protocol_options = g_list_append(irc_info->protocol_options, option);
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	purple_cmd_unregister(notice_cmd_id);

	return TRUE;
}

static PurplePluginPrefFrame *
get_plugin_pref_frame(PurplePlugin *plugin)
{
	PurplePluginPrefFrame *frame;
	PurplePluginPref *pref;

	frame = purple_plugin_pref_frame_new();

	pref = purple_plugin_pref_new_with_name_and_label(PREF_DELAY,
					_("Seconds to wait before rejoining"));
	purple_plugin_pref_set_bounds(pref, 3, 3600);
	purple_plugin_pref_frame_add(frame, pref);

	return frame;
}

static PurplePluginUiInfo prefs_info = {
	get_plugin_pref_frame,
	0,
	NULL,

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

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

	PLUGIN_ID,
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
	&prefs_info,
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

	purple_prefs_add_none(PREF_PREFIX);
	purple_prefs_add_int(PREF_DELAY, 30);
}

PURPLE_INIT_PLUGIN(irc_more, init_plugin, info)


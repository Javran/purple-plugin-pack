/**
 * @file ignore.c Ignore people.
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

/* Pack/Local headers */
#include "../common/pp_internal.h"

#include <account.h>
#include <blist.h>
#include <cmds.h>
#include <conversation.h>
#include <debug.h>
#include <plugin.h>

#include <string.h>

#define PREF_ROOT "/plugins/ignore"

static PurpleCmdId cmd;

typedef enum
{
	IGNORE_ALL     = 1 << 0,
	IGNORE_CHAT    = 1 << 1
} IgnoreContext;

static const char *
rule_key(PurpleAccount *account, const char *name)
{
	static char key[1024];

	snprintf(key, sizeof(key), PREF_ROOT "/%s/%s/%s",
			purple_account_get_protocol_id(account),
			purple_account_get_username(account),
			name);

	return key;
}

static void
add_ignore_rule(IgnoreContext context, const char *name, PurpleAccount *account)
{
	GString *string;
	char *pref;

	string = g_string_new(PREF_ROOT);
	string = g_string_append_c(string, '/');
	string = g_string_append(string, purple_account_get_protocol_id(account));
	if (!purple_prefs_exists(string->str))
		purple_prefs_add_none(string->str);
	string = g_string_append_c(string, '/');
	string = g_string_append(string, purple_account_get_username(account));
	if (!purple_prefs_exists(string->str))
		purple_prefs_add_none(string->str);
	string = g_string_append_c(string, '/');
	string = g_string_append(string, name);
	pref = g_ascii_strdown(string->str, string->len);
	if (!purple_prefs_exists(pref)) {
		GList *list = purple_prefs_get_string_list(PREF_ROOT "/rules");
		purple_prefs_add_string(pref, context == IGNORE_ALL ? "all" : "chat");
		if (!g_list_find_custom(list, pref, (GCompareFunc)g_utf8_collate)) {
			list = g_list_prepend(list, g_strdup(pref));
			purple_prefs_set_string_list(PREF_ROOT "/rules", list);
			g_list_foreach(list, (GFunc)g_free, NULL);
			g_list_free(list);
		}
	} else {
		purple_prefs_set_string(pref, context == IGNORE_ALL ? "all" : "chat");
	}
	g_string_free(string, TRUE);
	g_free(pref);
}

static void
remove_ignore_rule_d(const char *name, PurpleAccount *account)
{
	char *key = g_ascii_strdown(rule_key(account, name), -1);
	if (purple_prefs_exists(key))
		purple_prefs_set_string(key, "none");
	g_free(key);
}

static void
list_ignore_rules()
{
	GString *string;
	GList *list = purple_prefs_get_string_list(PREF_ROOT "/rules");
	char *last = NULL;

	string = g_string_new(NULL);
	list = g_list_sort(list, (GCompareFunc)g_utf8_collate);

	while (list) {
		char *pref = list->data;
		const char *rule = purple_prefs_get_string(pref);
		char *split = strrchr(pref, '/');
		*split++ = '\0';

		if (*rule != 'n') {
			if (last == NULL || g_strcasecmp(last, pref)) {
				g_free(last);
				last = g_strdup(pref);
				g_string_append_printf(string, "Ignore rules for %s<br>", last);
			}
			g_string_append_printf(string, "\t%s: %s<br>", split, rule);
		}
		g_free(list->data);
		list = g_list_delete_link(list, list);
	}
	purple_notify_formatted(NULL, _("Ignore Rules"), _("The following are the current ignore rules"),
			NULL, *string->str ? string->str : _("(Dear God! You are not ignoring any one!)"), NULL, NULL);
	g_string_free(string, TRUE);
	g_free(last);
}

static PurpleCmdRet
ignore_cmd(PurpleConversation *conv, const char *cmd, char **arguments, char **error, gpointer data)
{
	int nargs = 0;
	IgnoreContext context = IGNORE_ALL;
	PurpleAccount *account;
	const char *who;
	char **args = g_strsplit(arguments[0], " ", -1);

	if (args == NULL) {
		list_ignore_rules();
		goto end;
	}

	if (strcmp(args[nargs], "-c") == 0) {
		nargs++;
		context = IGNORE_CHAT;
	}

	if (args[nargs] == NULL) {
		goto end;
	}
#if 0
	if (isdigit(args[nargs][0])) {
		if (sscanf(args[nargs], "%d", &timer) == 1)
			nargs++;
	}
#endif
	account = purple_conversation_get_account(conv);

	who = args[nargs][1] ? (args[nargs] + 1) : purple_conversation_get_name(conv);
	while (args[nargs]) {
		switch(args[nargs][0]) {
			case '+':
				add_ignore_rule(context, who, account);
				break;
			case '-':
				remove_ignore_rule_d(who, account);
				break;
			default:
				purple_debug_warning("ignore", "invalid command %s\n", args[nargs]);
				break;
		}

		nargs++;
	}
end:
	g_strfreev(args);
	return PURPLE_CMD_STATUS_OK;
}

static gboolean
is_ignored(const char *name, PurpleAccount *account, PurpleConversationType type)
{
	char *key = g_ascii_strdown(rule_key(account, name), -1);
	const char *pref = NULL;

	if (!purple_prefs_exists(key)) {
		g_free(key);
		return FALSE;
	}
	pref = purple_prefs_get_string(key);
	g_free(key);
	if (!pref)
		return FALSE;

	if (*pref == 'a') {
		purple_debug_info("ignore", "ignoring %s\n", name);
		return TRUE;
	}

	if (*pref == 'c' && type == PURPLE_CONV_TYPE_CHAT) {
		purple_debug_info("ignore", "ignoring %s\n", name);
		return TRUE;
	}

	return FALSE;
}

static gboolean
receiving_msg(PurpleAccount *account, const char **who, const char **message,
		PurpleConversation *conv, PurpleMessageFlags *flags, PurpleConversationType type)
{
	return is_ignored(*who, account, type);
}

static gboolean
chat_activity_cb(PurpleConversation *conv, const char *user, PurpleConvChatBuddyFlags flags)
{
	return is_ignored(user, purple_conversation_get_account(conv), PURPLE_CONV_TYPE_CHAT);
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	cmd = purple_cmd_register("ignore", "s", PURPLE_CMD_P_DEFAULT,
				PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS, NULL,
				ignore_cmd, _("ignore [-c] [+&lt;ignore&gt; -&lt;unignore&gt;]<br>\
Examples:<br>\
   'ignore +StupidBot -NotABot' \t - (in a chat) Starts ignoring StupidBot, and removes NotABot from ignore list.<br>\
   'ignore -c +AnotherBot' \t - (in a chat) Starts ignoring AnotherBot, but only in chats.<br>\
   'ignore +' \t - (in an IM) Starts ignoring this person.<br>\
   'ignore -' \t - (in an IM) Starts unignoring this person.<br>\
   'ignore' \t - Lists the current ignore rules."), NULL);

	purple_signal_connect(purple_conversations_get_handle(), "chat-buddy-leaving", plugin,
			G_CALLBACK(chat_activity_cb), NULL);
	purple_signal_connect(purple_conversations_get_handle(), "chat-buddy-joining", plugin,
			G_CALLBACK(chat_activity_cb), NULL);
	purple_signal_connect(purple_conversations_get_handle(), "receiving-chat-msg", plugin,
			G_CALLBACK(receiving_msg), GINT_TO_POINTER(PURPLE_CONV_TYPE_CHAT));
	purple_signal_connect(purple_conversations_get_handle(), "receiving-im-msg", plugin,
			G_CALLBACK(receiving_msg), GINT_TO_POINTER(PURPLE_CONV_TYPE_IM));

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	purple_cmd_unregister(cmd);
	cmd = -1;
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
	"gnt-ignore",
	N_("Ignore"),
	VERSION,
	N_("Flexible plugin to selectively ignore people. Please do not use if you have amnesia."),
	N_("Flexible plugin to selectively ignore people. See '/help ignore' for more help.\nPlease do not use if you have amnesia."),
	"Sadrul H Chowdhury <sadrul@users.sourceforge.net>",
	PP_WEBSITE,
	plugin_load,
	plugin_unload,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	0,0,0,0
};

static void
init_plugin(PurplePlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	purple_prefs_add_none(PREF_ROOT);
	purple_prefs_add_string_list(PREF_ROOT "/rules", NULL);
}

PURPLE_INIT_PLUGIN(ignore, init_plugin, info)


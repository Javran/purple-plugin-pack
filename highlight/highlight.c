/**
 * highlight.c Highlight on customized words.
 *
 * Copyright (C) 2007-2008 Sadrul Habib Chowdhury <sadrul@users.sourceforge.net>
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

/* Pack/Local headers */
#include "../common/pp_internal.h"

#include <plugin.h>

#include <account.h>
#include <blist.h>
#include <cmds.h>
#include <conversation.h>
#include <debug.h>
#include <notify.h>
#include <util.h>

#include <string.h>

#define PREF_PREFIX "/plugins/core/highlight"
#define PREF_WORDS PREF_PREFIX "/words"

#define DELIMS " \t.,;|<>?/\\`~!@#$%^&*()+={}[]:'\""

#define PROP     "highlight-words"

static char **words;
static PurpleCmdId cmd;

/**
 * History stuff.
 */
/* XXX: SAVE THE NAME AND ACCOUNT OF THE CONVERSATION. OTHERWISE, IT CAUSES
 * A CRASH WHEN A HIGHLIGHTED CONVERSATION NO LONGER EXISTS.
 */
static GHashTable *history;

static void
string_destroy(gpointer data)
{
	g_string_free(data, TRUE);
}

static void
print_history_from_one_conv(gpointer key, gpointer value, gpointer data)
{
	g_string_append_printf(data, "<b>Highlights from %s (%s)<br>%s<br><br><hr>",
			purple_conversation_get_name(key),
			purple_account_get_username(purple_conversation_get_account(key)),
			((GString*)value)->str);
}

static void
print_history()
{
	GString *str = g_string_new(NULL);
	g_hash_table_foreach(history, print_history_from_one_conv, str);
	purple_notify_formatted(NULL, _("Highlight History"), _("Highlight History"),
			NULL, str->str, NULL, NULL);
	string_destroy(str);
}

static void
clear_history()
{
	g_hash_table_destroy(history);
	history = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, string_destroy);
}

static void
add_to_history(PurpleConversation *conv, const char *message, const char *who, time_t mtime)
{
	GString *str = g_hash_table_lookup(history, conv);
	if (str == NULL) {
		str = g_string_new(NULL);
		g_hash_table_replace(history, conv, str);
	}
	g_string_append_printf(str, "<br>(%s) <b>%s</b>: %s",
			purple_time_format(localtime(&mtime)),
			who, message);
}

/* End of history */

static void
casefold_collate_strings(char **string)
{
	int i;

	for (i = 0; string[i]; i++) {
		char *store = string[i];
		char *cs = g_utf8_casefold(store, -1);
		string[i] = g_utf8_collate_key(cs, -1);
		g_free(cs);
		g_free(store);
	}
}

static void
sort(char **strings, int length)
{
	int half;
	char **left, **middle, **m, **right;
	char **ret, **r;

	if (length <= 1)
		return;

	r = ret = g_new0(char *, length);

	half = length / 2;
	sort(strings, half);
	sort(strings + half, length - half);

	left = strings;
	middle = m = strings + half;
	right = strings + length;

	while (left < middle && m < right) {
		int comp = strcmp(*left, *m);
		if (comp <= 0)
			*r++ = *left++;
		else
			*r++ = *m++;
	}
	while (left < middle)
		*r++ = *left++;
	while (m < right)
		*r++ = *m++;
	for (half = 0; half < length; half++)
		strings[half] = ret[half];
	g_free(ret);
}

static gboolean
writing_msg_callback(PurpleAccount *account, char *who, char **message, PurpleConversation *conv,
		PurpleMessageFlags flags)
{
	if (flags & PURPLE_MESSAGE_NICK)
		add_to_history(conv, *message, who, time(NULL));
	return FALSE;
}

static gboolean
msg_callback(PurpleAccount *account, char **who, char **message, PurpleConversation *conv,
		PurpleMessageFlags *flags)
{
	char **splits;
	int len;
	int wl, sl;

	if (*flags & PURPLE_MESSAGE_NICK) {
		return FALSE;     /* this message is already highlighted */
	}

	if (!words)
		return FALSE;

	if (g_utf8_collate(*who, purple_connection_get_display_name(purple_account_get_connection(account))) == 0)
		return FALSE;

	splits = g_strsplit_set(*message, DELIMS, -1);
	if (!splits)
		return FALSE;
	len = 0;
	while (splits[len])
		len++;
	casefold_collate_strings(splits);
	sort(splits, len);

	/* this is probably over-engineering. */
	for (sl = 0, wl = 0; words[wl] && splits[sl]; ) {
		int val = strcmp(words[wl], splits[sl]);
		if (val == 0) {
			*flags |= PURPLE_MESSAGE_NICK;
			break;
		}
		if (val < 0)
			wl++;
		else
			sl++;
	}
	g_strfreev(splits);
	return FALSE;
}

static void
construct_list()
{
	int len = 0;
	g_strfreev(words);
	words = g_strsplit_set(purple_prefs_get_string(PREF_WORDS), DELIMS, -1);
	if (!words)
		return;
	while (words[len])
		len++;
	casefold_collate_strings(words);
	sort(words, len);
}

static PurpleCmdRet
highlight_cmd(PurpleConversation *conv, const gchar *cmd, gchar **args, gchar **error, void *data)
{
	if (g_utf8_collate(args[0], "history") == 0) {
		print_history();
	} else if (g_utf8_collate(args[0], "clear") == 0) {
		clear_history();
	} else {
	}
	return PURPLE_CMD_RET_OK;
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
#if !GLIB_CHECK_VERSION(2,4,0)
	return FALSE;
#else
	construct_list();

	purple_signal_connect(purple_conversations_get_handle(), "receiving-chat-msg", plugin,
			G_CALLBACK(msg_callback), NULL);
	purple_signal_connect(purple_conversations_get_handle(), "writing-chat-msg", plugin,
			G_CALLBACK(writing_msg_callback), NULL);
	purple_prefs_connect_callback(plugin, PREF_WORDS,
					(PurplePrefCallback)construct_list, NULL);

	cmd = purple_cmd_register("highlight", "ws", PURPLE_CMD_P_DEFAULT,
			PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS, NULL, highlight_cmd,
			_("/highlight history: shows the list of highlighted sentences from the history.\n"
			  "/highlight clear:   clears the history.\n"
			  "/highlight +&lt;word&gt;:  adds &lt;word&gt; to the highlight word list for this conversation only.\n"
			  "/highlight -&lt;word&gt;:  removes &lt;word&gt; from the highlight word list for this conversation only.\n"),
			NULL);

	history = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, string_destroy);
	return TRUE;
#endif
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	if (cmd)
		purple_cmd_unregister(cmd);
	g_hash_table_destroy(history);
	return TRUE;
}

static PurplePluginPrefFrame *
get_plugin_pref_frame(PurplePlugin *plugin)
{
	PurplePluginPrefFrame *frame;
	PurplePluginPref *pref;

	frame = purple_plugin_pref_frame_new();

	pref = purple_plugin_pref_new_with_name_and_label(PREF_WORDS, _("Words to highlight on\n"
						"(separate words by space)"));
	purple_plugin_pref_frame_add(frame, pref);

	return frame;
}

static PurplePluginUiInfo prefs_info = {
	get_plugin_pref_frame,
	0,
	NULL,
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
	"core-plugin_pack-highlight",
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
	purple_prefs_add_none(PREF_PREFIX);
	purple_prefs_add_string(PREF_WORDS, "");

	info.name = _("Highlight");
	info.summary = _("Support for highlighting words.");
	info.description = _("Support for highlighting words.");
}

PURPLE_INIT_PLUGIN(ignore, init_plugin, info)


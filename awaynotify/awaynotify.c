/*
 * awaynotify - show notices when status changes
 * Copyright (C) 2005-2006 Matt Perry <guy@somewhere.fscked.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
# include "../pp_config.h"
#endif

#include "../common/pp_internal.h"

#define PURPLE_PLUGINS

#include <blist.h>
#include <conversation.h>
#include <debug.h>
#include <signals.h>
#include <version.h>

#include <plugin.h>
#include <pluginpref.h>
#include <prefs.h>

#include <string.h>

#define PLUGIN_ID "core-plugin_pack-awaynotify"
#define CHECK_AWAY_MESSAGE_TIME_MS 1000

typedef struct _Infochecker Infochecker;

struct _Infochecker {
	PurpleAccount *account;
	char *buddy;
	guint timeout_id;
};

GList* infochecker_list = NULL;

static gint infocheck_timeout(gpointer data);

static Infochecker* infocheck_new(PurpleAccount* account, char* buddy)
{
	Infochecker* checker = g_new0(Infochecker, 1);
	checker->account = account;
	checker->buddy = g_strdup(buddy);

	return checker;
}

static void infocheck_delete(Infochecker* checker)
{
	g_free(checker->buddy);
	g_free(checker);
}

static void infocheck_remove(GList* node)
{
	Infochecker* checker = (Infochecker*)node->data;

	g_source_remove(checker->timeout_id);
	infochecker_list = g_list_remove_link(infochecker_list, node);

	infocheck_delete(checker);
}

static gint infocheck_compare(gconstpointer pa, gconstpointer pb)
{
	Infochecker* a = (Infochecker*)pa;
	Infochecker* b = (Infochecker*)pb;

	return (a->account == b->account) ? strcmp(a->buddy, b->buddy) : 1;
}


static void write_status(PurpleBuddy *buddy, const char *message, const char* status)
{
	PurpleConversation *conv;
	const char *who;
	char buf[256];
	char *escaped;

	conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, buddy->name, buddy->account);

	if (conv == NULL)
		return;

	who = purple_buddy_get_alias(buddy);
	escaped = g_markup_escape_text(who, -1);

	g_snprintf(buf, sizeof(buf), message, escaped, status);
	g_free(escaped);

	purple_conversation_write(conv, NULL, buf, PURPLE_MESSAGE_SYSTEM, time(NULL));
}

static char* parse_away_message(char* statustext)
{
	char* away_ptr = strstr(statustext, "Away Message:");

	if (away_ptr == NULL)
		return g_strdup("");

	away_ptr += 4 + 1 + 7 + 1;
	if (*away_ptr == '<') {
		char* tmp = strchr(away_ptr, '>');
		if (tmp) away_ptr = tmp + 1;
	}

	while (*away_ptr == ' ') away_ptr++;

	return g_strdup(away_ptr);
}

static char* get_away_message(PurpleBuddy* buddy)
{
	PurplePlugin *prpl;
	PurplePluginProtocolInfo *prpl_info = NULL;
	char* statustext = NULL;

	if (buddy == NULL)
		return NULL;

	prpl = purple_find_prpl(purple_account_get_protocol_id(buddy->account));
	prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(prpl);

	if (prpl_info && prpl_info->tooltip_text) {
		const char *end;
		char *statustext = NULL;
		PurpleNotifyUserInfo *info = purple_notify_user_info_new();
		prpl_info->tooltip_text(buddy, info, TRUE);
		statustext = purple_notify_user_info_get_text_with_newline(info, "\n");
		purple_notify_user_info_destroy(info);

		if (statustext && !g_utf8_validate(statustext, -1, &end)) {
			char *new = g_strndup(statustext, g_utf8_pointer_to_offset(statustext, end));
			g_free(statustext);
			statustext = new;
		}
	}

	if (statustext) {
		char* away_message = parse_away_message(statustext);
		g_free(statustext);
		return away_message;
	} else
		return NULL;
}

static gint infocheck_timeout(gpointer data)
{
	GList* node = (GList*)data;
	Infochecker* checker = node ? (Infochecker*)node->data : NULL;
	PurpleBuddy* buddy;
	char* away_message;

	if (node == NULL || checker == NULL) {
		purple_debug_warning("awaynotify", "checker called without being active!\n");
		return FALSE;
	}

	buddy = purple_find_buddy(checker->account, checker->buddy);
	away_message = get_away_message(buddy);

	if (away_message == NULL) {
		/* He must have signed off or there was some other error.  Give up. */
		infocheck_remove(node);
		return FALSE;
	}

	if (away_message[0] == 0) {
		/* Not away yet.  Return true to try again. */
		g_free(away_message);
		return TRUE;
	}

	write_status(buddy, _("%s is away: %s"), away_message);

	g_free(away_message);
	infocheck_remove(node);

	return FALSE;
}

static void infocheck_add(Infochecker* checker)
{
	infochecker_list = g_list_prepend(infochecker_list, checker);
	checker->timeout_id = g_timeout_add(CHECK_AWAY_MESSAGE_TIME_MS,
			infocheck_timeout, g_list_first(infochecker_list));
}

static void buddy_away_cb(PurpleBuddy *buddy, void *data)
{
	if (purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, buddy->name, buddy->account) == NULL)
		return; /* Ignore if there's no conv open. */

	infocheck_add(infocheck_new(buddy->account, buddy->name));
}

static void buddy_unaway_cb(PurpleBuddy *buddy, void *data)
{
	GList* node = g_list_find_custom(infochecker_list, buddy->name, infocheck_compare);

	if (node)
		infocheck_remove(node);

	write_status(buddy, _("%s is no longer away."), NULL);
}

static gboolean plugin_load(PurplePlugin *plugin)
{
	void *blist_handle = purple_blist_get_handle();

	purple_signal_connect(blist_handle, "buddy-away",
						plugin, PURPLE_CALLBACK(buddy_away_cb), NULL);
	purple_signal_connect(blist_handle, "buddy-back",
						plugin, PURPLE_CALLBACK(buddy_unaway_cb), NULL);

	return TRUE;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,                             /**< type           */
	NULL,                                             /**< ui_requirement */
	0,                                                /**< flags          */
	NULL,                                             /**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,                            /**< priority       */

	PLUGIN_ID,      			                      /**< id             */
	NULL,                                             /**< name           */
	PP_VERSION,                                      /**< version        */
	NULL,                                             /**  summary        */
	NULL,                                             /**  description    */
	"Matt Perry <guy@somewhere.fscked.org>",          /**< author         */
	PP_WEBSITE,                                      /**< homepage       */

	plugin_load,                                      /**< load           */
	NULL,                                             /**< unload         */
	NULL,                                             /**< destroy        */

	NULL,                                             /**< ui_info        */
	NULL,                                             /**< extra_info     */
	NULL, 		                                      /**< prefs_info     */
	NULL
};

static void
init_plugin(PurplePlugin *plugin)
{
	info.name = _("Away State Notification");
	info.summary =
		_("Notifies in a conversation window when a buddy goes or returns from away");
	info.description = info.summary;
}

PURPLE_INIT_PLUGIN(statenotify, init_plugin, info)


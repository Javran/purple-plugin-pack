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
# include "../gpp_config.h"
#endif

#include "blist.h"
#include "conversation.h"
#include "debug.h"
#include "signals.h"
#include "version.h"

#include "plugin.h"
#include "pluginpref.h"
#include "prefs.h"

#define PLUGIN_ID "core-plugin_pack-awaynotify"
#define CHECK_AWAY_MESSAGE_TIME_MS 1000

typedef struct _Infochecker Infochecker;

struct _Infochecker {
	GaimAccount *account;
	char *buddy;
	guint timeout_id;
};

GList* infochecker_list = NULL;

static Infochecker* infocheck_new(GaimAccount* account, char* buddy)
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

static void infocheck_add(Infochecker* checker)
{
	static gint infocheck_timeout(gpointer data);

	infochecker_list = g_list_prepend(infochecker_list, checker);
	checker->timeout_id = g_timeout_add(CHECK_AWAY_MESSAGE_TIME_MS,
			infocheck_timeout, g_list_first(infochecker_list));
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


static void write_status(GaimBuddy *buddy, const char *message, const char* status)
{
	GaimConversation *conv;
	const char *who;
	char buf[256];
	char *escaped;

	conv = gaim_find_conversation_with_account(buddy->name, buddy->account);

	if (conv == NULL)
		return;

	who = gaim_buddy_get_alias(buddy);
	escaped = g_markup_escape_text(who, -1);

	g_snprintf(buf, sizeof(buf), message, escaped, status);
	g_free(escaped);

	gaim_conversation_write(conv, NULL, buf, GAIM_MESSAGE_SYSTEM, time(NULL));
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

static char* get_away_message(GaimBuddy* buddy)
{
	GaimPlugin *prpl;
	GaimPluginProtocolInfo *prpl_info = NULL;
	char* statustext = NULL;

	if (buddy == NULL)
		return NULL;

	prpl = gaim_find_prpl(gaim_account_get_protocol_id(buddy->account));
	prpl_info = GAIM_PLUGIN_PROTOCOL_INFO(prpl);

	if (prpl_info && prpl_info->tooltip_text) {
		const char *end;
		statustext = prpl_info->tooltip_text(buddy);

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
	GaimBuddy* buddy;
	char* away_message;

	if (node == NULL || checker == NULL) {
		gaim_debug_warning("awaynotify", "checker called without being active!\n");
		return FALSE;
	}

	buddy = gaim_find_buddy(checker->account, checker->buddy);
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

static void buddy_away_cb(GaimBuddy *buddy, void *data)
{
	if (gaim_find_conversation_with_account(buddy->name, buddy->account) == NULL)
		return; /* Ignore if there's no conv open. */

	infocheck_add(infocheck_new(buddy->account, buddy->name));
}

static void buddy_unaway_cb(GaimBuddy *buddy, void *data)
{
	GList* node = g_list_find_custom(infochecker_list, buddy->name, infocheck_compare);

	if (node)
		infocheck_remove(node);

	write_status(buddy, _("%s is no longer away."), NULL);
}

static gboolean plugin_load(GaimPlugin *plugin)
{
	void *blist_handle = gaim_blist_get_handle();

	gaim_signal_connect(blist_handle, "buddy-away",
						plugin, GAIM_CALLBACK(buddy_away_cb), NULL);
	gaim_signal_connect(blist_handle, "buddy-back",
						plugin, GAIM_CALLBACK(buddy_unaway_cb), NULL);

	return TRUE;
}

static GaimPluginInfo info =
{
	GAIM_PLUGIN_MAGIC,
	GAIM_MAJOR_VERSION,
	GAIM_MINOR_VERSION,
	GAIM_PLUGIN_STANDARD,                             /**< type           */
	NULL,                                             /**< ui_requirement */
	0,                                                /**< flags          */
	NULL,                                             /**< dependencies   */
	GAIM_PRIORITY_DEFAULT,                            /**< priority       */

	PLUGIN_ID,      			                      /**< id             */
	NULL,                                             /**< name           */
	GPP_VERSION,                                      /**< version        */
	NULL,                                             /**  summary        */
	NULL,                                             /**  description    */
	"Matt Perry <guy@somewhere.fscked.org>",          /**< author         */
	GPP_WEBSITE,                                      /**< homepage       */

	plugin_load,                                      /**< load           */
	NULL,                                             /**< unload         */
	NULL,                                             /**< destroy        */

	NULL,                                             /**< ui_info        */
	NULL,                                             /**< extra_info     */
	NULL, 		                                      /**< prefs_info     */
	NULL
};

static void
init_plugin(GaimPlugin *plugin)
{
	info.name = _("Away State Notification");
	info.summary =
		_("Notifies in a conversation window when a buddy goes or returns from away");
	info.description = info.summary;
}

GAIM_INIT_PLUGIN(statenotify, init_plugin, info)


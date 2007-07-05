/*
 * Last Seen - Record when a buddy was last seen
 * Copyright (C) 2004 Stu Tomlinson <stu@nosnilmot.com>
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
#endif /* HAVE_CONFIG_H */

#define PURPLE_PLUGINS

#include <plugin.h>
#include <debug.h>
#include <blist.h>
#include <signals.h>
#include <util.h>
#include <version.h>

#include <gtkplugin.h>
#include <gtkblist.h>

#include "../common/pp_internal.h"

/* global list of accounts connecting, to avoid inaccurate signon times
 * idea stolen from guifications :) - thanks Gary.
 */
GSList *connecting;

static gboolean
remove_connecting_account(void *p)
{
	PurpleAccount *account = (PurpleAccount *)p;

	if (account == NULL)
		return FALSE;

	if(account->gc == NULL)
	{
		connecting = g_slist_remove(connecting, account);
		return FALSE;
	}

	if (account->gc->state == PURPLE_CONNECTING)
		return TRUE;

	connecting = g_slist_remove(connecting, account);

	return FALSE;
}

static void
account_connecting_cb(PurpleAccount *account, void *data)
{
	if (g_slist_find(connecting, account) == NULL)
	{
		connecting = g_slist_append(connecting, account);
		gtk_timeout_add(10000, remove_connecting_account, account);
	}
}

static void
received_im_msg_cb(PurpleAccount *account, char *sender, char *message,
				   PurpleConversation *conv, int flags, void *data)
{
	PurpleBuddy *buddy;
	gchar *said = NULL;
	GSList *buds;

	purple_markup_html_to_xhtml(message, NULL, &said);
	for (buds = purple_find_buddies(account, sender); buds; buds = buds->next)
	{
		buddy = buds->data;

		purple_blist_node_set_int((PurpleBlistNode*)buddy, "lastseen", time(NULL));
		purple_blist_node_set_string((PurpleBlistNode*)buddy, "lastsaid", g_strchomp(said));
	}
	g_free(said);
}

static void
buddy_signedon_cb(PurpleBuddy *buddy, void *data)
{
	/* TODO: if the buddy is actually the account that is signing on, we
	 * should actually update signon time. Don't forget to normalize
	 * sceennames before comparing them! Also, what do we do if the buddy
	 * is always online when we log in? hmmm */
	if (g_slist_find(connecting, buddy->account) != NULL)
		return;
	purple_blist_node_set_int((PurpleBlistNode*)buddy, "signedon", time(NULL));
}

static void
buddy_signedoff_cb(PurpleBuddy *buddy, void *data)
{
	purple_blist_node_set_int((PurpleBlistNode*)buddy, "signedoff", time(NULL) );
}

static void
drawing_tooltip_cb(PurpleBlistNode *node, GString *str, gboolean full, void *data)
{
	PurpleBuddy *buddy = NULL;
	PurpleBlistNode *n;
	time_t last = 0, max = 0, off = 0, on = 0;
	const gchar *tmp = NULL;
	gchar *seen = NULL, *said = NULL, *offs = NULL, *ons = NULL;

	if(PURPLE_BLIST_NODE_IS_BUDDY(node))
	{
		if (!full)
			return;
		node = (PurpleBlistNode *)purple_buddy_get_contact((PurpleBuddy *)node);
	}

	if(!PURPLE_BLIST_NODE_IS_CONTACT(node))
		return;

	for (n = node->child; n; n = n->next)
	{
		if (!PURPLE_BLIST_NODE_IS_BUDDY(n))
			continue;
		last = purple_blist_node_get_int(n, "lastseen");
		if (last > max) {
			max = last;
			buddy = (PurpleBuddy *)n;
		}
		last = 0;
	}

	if(!buddy)
		buddy = purple_contact_get_priority_buddy((PurpleContact *)node);

	last = purple_blist_node_get_int((PurpleBlistNode *)buddy, "lastseen");
	if (last)
		seen = purple_str_seconds_to_string(time(NULL) - last);
	on = purple_blist_node_get_int((PurpleBlistNode *)buddy, "signedon");
	if (on)
		ons = purple_str_seconds_to_string(time(NULL) - on);
	if(!PURPLE_BUDDY_IS_ONLINE(buddy)) {
		off = purple_blist_node_get_int((PurpleBlistNode *)buddy, "signedoff");
		if (off)
			offs = purple_str_seconds_to_string(time(NULL) - off);
	}
	tmp = purple_blist_node_get_string((PurpleBlistNode *)buddy, "lastsaid");
	if(tmp)
		said = g_strchomp(g_markup_escape_text(tmp, -1));

	g_string_append_printf(str,
					  "%s%s" /* Last seen */
					  "%s%s" /* Last said */
					  "%s%s" /* Signed on */
					  "%s%s", /* Signed off */
					  seen ? _("\n<b>Last Seen</b>: ") : "", seen ? seen : "",
					  said ? _("\n<b>Last Said</b>: ") : "", said ? said : "",
					  ons ? _("\n<b>Signed On</b>: ") : "", ons ? ons : "",
					  offs ? _("\n<b>Signed Off</b>: ") : "", offs ? offs : "");
	g_free(seen);
	g_free(said);
	g_free(ons);
	g_free(offs);
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	void *blist = purple_blist_get_handle();
	void *conversation = purple_conversations_get_handle();
	void *gtkblist = pidgin_blist_get_handle();

	connecting = NULL;

	purple_signal_connect(purple_accounts_get_handle(), "account-connecting",
						plugin, PURPLE_CALLBACK(account_connecting_cb), NULL);
	purple_signal_connect(conversation, "received-im-msg",
						plugin, PURPLE_CALLBACK(received_im_msg_cb), NULL);
	purple_signal_connect(blist, "buddy-signed-on", plugin,
						PURPLE_CALLBACK(buddy_signedon_cb), NULL);
	purple_signal_connect(blist, "buddy-signed-off", plugin,
						PURPLE_CALLBACK(buddy_signedoff_cb), NULL);
	purple_signal_connect(gtkblist, "drawing-tooltip",
						plugin, PURPLE_CALLBACK(drawing_tooltip_cb), NULL);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	g_slist_free(connecting);

	return TRUE;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,								/**< magic			*/
	PURPLE_MAJOR_VERSION,								/**< major version	*/
	PURPLE_MINOR_VERSION,								/**< minor version	*/
	PURPLE_PLUGIN_STANDARD,							/**< type			*/
	PIDGIN_PLUGIN_TYPE,							/**< ui_requirement	*/
	0,												/**< flags			*/
	NULL,											/**< dependencies	*/
	PURPLE_PRIORITY_DEFAULT,							/**< priority		*/

	"gtk-plugin_pack-lastseen",						/**< id				*/
	NULL,											/**< name			*/
	PP_VERSION,									/**< version		*/
	NULL,											/**  summary		*/
	NULL,											/**  description	*/
	"Stu Tomlinson <stu@nosnilmot.com>",			/**< author			*/
	PP_WEBSITE,									/**< homepage		*/

	plugin_load,									/**< load			*/
	plugin_unload,									/**< unload			*/
	NULL,											/**< destroy		*/

	NULL,											/**< ui_info		*/
	NULL,											/**< extra_info		*/
	NULL,											/**< prefs_info		*/
	NULL,											/**< actions		*/
	NULL,											/**< reserved 1		*/
	NULL,											/**< reserved 2		*/
	NULL,											/**< reserved 3		*/
	NULL											/**< reserved 4		*/
};

static void
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Last Seen");
	info.summary = _("Record when a buddy was last seen.");
	info.description = _("Logs the time of a last received message, what "
						 "they said, when they logged in, and when they "
						 "logged out, for buddies on your buddy list.");
}

PURPLE_INIT_PLUGIN(lastseen, init_plugin, info)

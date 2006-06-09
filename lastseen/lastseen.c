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
#if HAVE_CONFIG_H
# include "../gpp_config.h"
#endif /* HAVE_CONFIG_H */

#define GAIM_PLUGINS

#include <plugin.h>
#include <debug.h>
#include <blist.h>
#include <signals.h>
#include <util.h>
#include <version.h>

#include <gtkplugin.h>
#include <gtkblist.h>

#include "../common/i18n.h"

/* global list of accounts connecting, to avoid inaccurate signon times
 * idea stolen from guifications :) - thanks Gary.
 */
GSList *connecting;

static gboolean
remove_connecting_account(void *p)
{
	GaimAccount *account = (GaimAccount *)p;

	if (account == NULL)
		return FALSE;

	if(account->gc == NULL)
	{
		connecting = g_slist_remove(connecting, account);
		return FALSE;
	}

	if (account->gc->state == GAIM_CONNECTING)
		return TRUE;

	connecting = g_slist_remove(connecting, account);

	return FALSE;
}

static void
account_connecting_cb(GaimAccount *account, void *data)
{
	if (g_slist_find(connecting, account) == NULL)
	{
		connecting = g_slist_append(connecting, account);
		gtk_timeout_add(10000, remove_connecting_account, account);
	}
}

static void
received_im_msg_cb(GaimAccount *account, char *sender, char *message,
				   GaimConversation *conv, int flags, void *data)
{
	GaimBuddy *buddy;
	gchar *said = NULL;
	GSList *buds;

	gaim_markup_html_to_xhtml(message, NULL, &said);
	for (buds = gaim_find_buddies(account, sender); buds; buds = buds->next)
	{
		buddy = buds->data;

		gaim_blist_node_set_int((GaimBlistNode*)buddy, "lastseen", time(NULL));
		gaim_blist_node_set_string((GaimBlistNode*)buddy, "lastsaid", g_strchomp(said));
	}
	g_free(said);
}

static void
buddy_signedon_cb(GaimBuddy *buddy, void *data)
{
	/* TODO: if the buddy is actually the account that is signing on, we
	 * should actually update signon time. Don't forget to normalize
	 * sceennames before comparing them! Also, what do we do if the buddy
	 * is always online when we log in? hmmm */
	if (g_slist_find(connecting, buddy->account) != NULL)
		return;
	gaim_blist_node_set_int((GaimBlistNode*)buddy, "signedon", time(NULL));
}

static void
buddy_signedoff_cb(GaimBuddy *buddy, void *data)
{
	gaim_blist_node_set_int((GaimBlistNode*)buddy, "signedoff", time(NULL) );
}

#if GAIM_VERSION_CHECK(2,0,0)
static void
drawing_tooltip_cb(GaimBlistNode *node, GString *str, gboolean full, void *data)
#else
static void
drawing_tooltip_cb(GaimBlistNode *node, char **text, void *data)
#endif
{
	GaimBuddy *buddy = NULL;
	GaimBlistNode *n;
	time_t last = 0, max = 0, off = 0, on = 0;
	const gchar *tmp = NULL;
	gchar *seen = NULL, *said = NULL, *offs = NULL, *ons = NULL;
#if !GAIM_VERSION_CHECK(2,0,0)
	gchar *tmp2 = NULL;
#endif

	if(GAIM_BLIST_NODE_IS_BUDDY(node))
	{
#if GAIM_VERSION_CHECK(2,0,0)
		if (!full)
			return;
#endif
		node = (GaimBlistNode *)gaim_buddy_get_contact((GaimBuddy *)node);
	}

	if(!GAIM_BLIST_NODE_IS_CONTACT(node))
		return;

	for (n = node->child; n; n = n->next)
	{
		if (!GAIM_BLIST_NODE_IS_BUDDY(n))
			continue;
		last = gaim_blist_node_get_int(n, "lastseen");
		if (last > max) {
			max = last;
			buddy = (GaimBuddy *)n;
		}
		last = 0;
	}

	if(!buddy)
		buddy = gaim_contact_get_priority_buddy((GaimContact *)node);

	last = gaim_blist_node_get_int((GaimBlistNode *)buddy, "lastseen");
	if (last)
		seen = gaim_str_seconds_to_string(time(NULL) - last);
	on = gaim_blist_node_get_int((GaimBlistNode *)buddy, "signedon");
	if (on)
		ons = gaim_str_seconds_to_string(time(NULL) - on);
	if(!GAIM_BUDDY_IS_ONLINE(buddy)) {
		off = gaim_blist_node_get_int((GaimBlistNode *)buddy, "signedoff");
		if (off)
			offs = gaim_str_seconds_to_string(time(NULL) - off);
	}
	tmp = gaim_blist_node_get_string((GaimBlistNode *)buddy, "lastsaid");
	if(tmp)
		said = g_strchomp(g_markup_escape_text(tmp, -1));

#if GAIM_VERSION_CHECK(2,0,0)
	g_string_append_printf(str,
					  "%s %s" /* Last seen */
					  "%s %s" /* Last said */
					  "%s %s" /* Signed on */
					  "%s %s", /* Signed off */
					  seen ? _("\n<b>Last Seen:</b>") : "", seen ? seen : "",
					  said ? _("\n<b>Last Said:</b>") : "", said ? said : "",
					  ons ? _("\n<b>Signed On:</b>") : "", ons ? ons : "",
					  offs ? _("\n<b>Signed Off:</b>") : "", offs ? offs : "");
#else
	tmp2 = g_strdup(*text);
	*text = g_strdup_printf("%s" /* existing */
					  "%s %s" /* Last seen */
					  "%s %s" /* Last said */
					  "%s %s" /* Signed on */
					  "%s %s", /* Signed off */
					  tmp2,
					  seen ? _("\n<b>Last Seen:</b>") : "", seen ? seen : "",
					  said ? _("\n<b>Last Said:</b>") : "", said ? said : "",
					  ons ? _("\n<b>Signed On:</b>") : "", ons ? ons : "",
					  offs ? _("\n<b>Signed Off:</b>") : "", offs ? offs : "");

	g_free(tmp2);
#endif
	g_free(seen);
	g_free(said);
	g_free(ons);
	g_free(offs);
}

static gboolean
plugin_load(GaimPlugin *plugin)
{
	void *blist = gaim_blist_get_handle();
	void *conversation = gaim_conversations_get_handle();
	void *gtkblist = gaim_gtk_blist_get_handle();

	connecting = NULL;

	gaim_signal_connect(gaim_accounts_get_handle(), "account-connecting",
						plugin, GAIM_CALLBACK(account_connecting_cb), NULL);
	gaim_signal_connect(conversation, "received-im-msg",
						plugin, GAIM_CALLBACK(received_im_msg_cb), NULL);
	gaim_signal_connect(blist, "buddy-signed-on", plugin,
						GAIM_CALLBACK(buddy_signedon_cb), NULL);
	gaim_signal_connect(blist, "buddy-signed-off", plugin,
						GAIM_CALLBACK(buddy_signedoff_cb), NULL);
	gaim_signal_connect(gtkblist, "drawing-tooltip",
						plugin, GAIM_CALLBACK(drawing_tooltip_cb), NULL);

	return TRUE;
}

static gboolean
plugin_unload(GaimPlugin *plugin)
{
	g_slist_free(connecting);

	return TRUE;
}

static GaimPluginInfo info =
{
	GAIM_PLUGIN_MAGIC,								/**< magic			*/
	GAIM_MAJOR_VERSION,								/**< major version	*/
	GAIM_MINOR_VERSION,								/**< minor version	*/
	GAIM_PLUGIN_STANDARD,							/**< type			*/
	GAIM_GTK_PLUGIN_TYPE,							/**< ui_requirement	*/
	0,												/**< flags			*/
	NULL,											/**< dependencies	*/
	GAIM_PRIORITY_DEFAULT,							/**< priority		*/

	"gtk-plugin_pack-lastseen",						/**< id				*/
	NULL,											/**< name			*/
	GPP_VERSION,									/**< version		*/
	NULL,											/**  summary		*/
	NULL,											/**  description	*/
	"Stu Tomlinson <stu@nosnilmot.com>",			/**< author			*/
	GPP_WEBSITE,									/**< homepage		*/

	plugin_load,									/**< load			*/
	plugin_unload,									/**< unload			*/
	NULL,											/**< destroy		*/

	NULL,											/**< ui_info		*/
	NULL,											/**< extra_info		*/
	NULL,											/**< prefs_info		*/
	NULL											/**< actions		*/
};

static void
init_plugin(GaimPlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GPP_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GPP_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Last Seen");
	info.summary = _("Record when a buddy was last seen.");
	info.description = _("Logs the time of a last received message, what "
						 "they said, when they logged in, and when they "
						 "logged out, for buddies on your buddy list.");
}

GAIM_INIT_PLUGIN(lastseen, init_plugin, info)

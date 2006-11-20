/*
 * GroupMsg - Send an IM to a group of buddies
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
# include "../gpp_config.h"
#endif /* HAVE_CONFIG_H */

#define GAIM_PLUGINS

#include <debug.h>
#include <notify.h>
#include <prpl.h>
#include <request.h>
#include <signals.h>
#include <util.h>
#include <version.h>

#include "../common/i18n.h"

static void
dont_do_it_cb(GList *list, const char *text)
{
	g_list_free(list);
}

static void
do_it_cb(GList *list, const char *message)
{
	char *stripped;
	GList *l;
	GaimBuddy *b;
	GaimConversation *conv = NULL;
#if !GAIM_VERSION_CHECK(2,0,0)
	GaimConvWindow *win;
#endif

	gaim_markup_html_to_xhtml(message, NULL, &stripped);

	for (l = list; l; l = l->next) {
		b = l->data;
		conv = gaim_conversation_new(GAIM_CONV_TYPE_IM, b->account, b->name);
		if(conv) {
			if (b->account->gc->flags & GAIM_CONNECTION_HTML)
				gaim_conv_im_send(GAIM_CONV_IM(conv), message);
			else
				gaim_conv_im_send(GAIM_CONV_IM(conv), stripped);
		}
	}
#if !GAIM_VERSION_CHECK(2,0,0)
	if(conv) {
		win = gaim_conversation_get_window(conv);
		gaim_conv_window_raise(win);
	}
#endif

	g_free(stripped);
	g_list_free(list);
}

static void
groupmsg_sendto_group(GaimBlistNode *node, gpointer data)
{
	GaimBlistNode *n;
	GaimBuddy *b;
	GaimGroup *group;
	GList *list = NULL;
	gchar *tmp = NULL, *tmp2 = NULL;

	group = (GaimGroup *)node;

	for (n = node->child; n; n = n->next) {

		if (GAIM_BLIST_NODE_IS_CHAT(n))
			continue;

		if (GAIM_BLIST_NODE_IS_CONTACT(n)) {
			b = gaim_contact_get_priority_buddy((GaimContact*)n);
		} else
			b = (GaimBuddy *)n;

		if(b == NULL || !GAIM_BUDDY_IS_ONLINE(b))
			continue;

		tmp2 = tmp;
		if(tmp != NULL)
			tmp = g_strdup_printf("   %s (%s)\n%s", gaim_buddy_get_alias(b), b->name, tmp2);
		else
			tmp = g_strdup_printf("   %s (%s)", gaim_buddy_get_alias(b), b->name);
		g_free(tmp2);
		list = g_list_append(list, b);

	}

	if (tmp == NULL) {
		tmp = g_strdup_printf(_("There are no buddies online in group %s"), group->name);
		gaim_notify_error(NULL, NULL, "No buddies", tmp);
		g_free(tmp);
		g_list_free(list);
		return;
	}

	tmp2 = tmp;
	tmp = g_strdup_printf(_("Your message will be sent to these buddies:\n%s"), tmp2);
	g_free(tmp2);

	gaim_request_input(group, _("Spam"),
					   _("Please enter the message to send"),
					   tmp,
					   "", TRUE, FALSE, "html",
					   _("Send"), G_CALLBACK(do_it_cb),
					   _("Cancel"), G_CALLBACK(dont_do_it_cb),
					   list);
	g_free(tmp);
}

static void
groupmsg_extended_menu_cb(GaimBlistNode *node, GList **m)
{
	GaimMenuAction *bna = NULL;

	if(!GAIM_BLIST_NODE_IS_GROUP(node))
		return;

	*m = g_list_append(*m, bna);
	bna = gaim_menu_action_new("Group IM", GAIM_CALLBACK(groupmsg_sendto_group), NULL, NULL);
	*m = g_list_append(*m, bna);
}

static gboolean
plugin_load(GaimPlugin *plugin)
{

	gaim_signal_connect(gaim_blist_get_handle(), "blist-node-extended-menu",
						plugin, GAIM_CALLBACK(groupmsg_extended_menu_cb), NULL);

	return TRUE;
}

static GaimPluginInfo info =
{
	GAIM_PLUGIN_MAGIC,								/**< magic			*/
	GAIM_MAJOR_VERSION,								/**< major version	*/
	GAIM_MINOR_VERSION,								/**< minor version	*/
	GAIM_PLUGIN_STANDARD,							/**< type			*/
	NULL,											/**< ui_requirement	*/
	0,												/**< flags			*/
	NULL,											/**< dependencies	*/
	GAIM_PRIORITY_DEFAULT,							/**< priority		*/

	"core-plugin_pack-groupmsg",					/**< id				*/
	NULL,											/**< name			*/
	GPP_VERSION,									/**< version		*/
	NULL,											/**  summary		*/
	NULL,											/**  description	*/
	"Stu Tomlinson <stu@nosnilmot.com>",			/**< author			*/
	GPP_WEBSITE,									/**< homepage		*/

	plugin_load,									/**< load			*/
	NULL,											/**< unload			*/
	NULL,											/**< destroy		*/

	NULL,											/**< ui_info		*/
	NULL,											/**< extra_info		*/
	NULL,											/**< prefs_info		*/
	NULL											/**< actions		*/
};


static void
init_plugin(GaimPlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Group IM");
	info.summary = _("Send an IM to a group of buddies.");
	info.description = _("Adds the option to send an IM to every online "
						 "buddy in a group.");
}

GAIM_INIT_PLUGIN(groupmsg, init_plugin, info)

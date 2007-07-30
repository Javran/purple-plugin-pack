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
# include "../pp_config.h"
#endif /* HAVE_CONFIG_H */

#define PURPLE_PLUGINS

#include "../common/pp_internal.h"

#include <debug.h>
#include <notify.h>
#include <prpl.h>
#include <request.h>
#include <signals.h>
#include <util.h>
#include <version.h>

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
	PurpleBuddy *b;
	PurpleConversation *conv = NULL;
#if !PURPLE_VERSION_CHECK(2,0,0)
	PurpleConvWindow *win;
#endif

	purple_markup_html_to_xhtml(message, NULL, &stripped);

	for (l = list; l; l = l->next) {
		b = l->data;
		conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, b->account, b->name);
		if(conv) {
			if (b->account->gc->flags & PURPLE_CONNECTION_HTML)
				purple_conv_im_send(PURPLE_CONV_IM(conv), message);
			else
				purple_conv_im_send(PURPLE_CONV_IM(conv), stripped);
		}
	}
#if !PURPLE_VERSION_CHECK(2,0,0)
	if(conv) {
		win = purple_conversation_get_window(conv);
		purple_conv_window_raise(win);
	}
#endif

	g_free(stripped);
	g_list_free(list);
}

static void
groupmsg_sendto_group(PurpleBlistNode *node, gpointer data)
{
	PurpleBlistNode *n;
	PurpleBuddy *b;
	PurpleGroup *group;
	GList *list = NULL;
	gchar *tmp = NULL, *tmp2 = NULL;

	group = (PurpleGroup *)node;

	for (n = node->child; n; n = n->next) {

		if (PURPLE_BLIST_NODE_IS_CHAT(n))
			continue;

		if (PURPLE_BLIST_NODE_IS_CONTACT(n)) {
			b = purple_contact_get_priority_buddy((PurpleContact*)n);
		} else
			b = (PurpleBuddy *)n;

		if(b == NULL || !PURPLE_BUDDY_IS_ONLINE(b))
			continue;

		tmp2 = tmp;
		if(tmp != NULL)
			tmp = g_strdup_printf("   %s (%s)\n%s", purple_buddy_get_alias(b), b->name, tmp2);
		else
			tmp = g_strdup_printf("   %s (%s)", purple_buddy_get_alias(b), b->name);
		g_free(tmp2);
		list = g_list_append(list, b);

	}

	if (tmp == NULL) {
		tmp = g_strdup_printf(_("There are no buddies online in group %s"), group->name);
		purple_notify_error(NULL, NULL, "No buddies", tmp);
		g_free(tmp);
		g_list_free(list);
		return;
	}

	tmp2 = tmp;
	tmp = g_strdup_printf(_("Your message will be sent to these buddies:\n%s"), tmp2);
	g_free(tmp2);

	purple_request_input(group, _("Spam"),
					   _("Please enter the message to send"),
					   tmp,
					   "", TRUE, FALSE, "html",
					   _("Send"), G_CALLBACK(do_it_cb),
					   _("Cancel"), G_CALLBACK(dont_do_it_cb),
					   NULL, NULL, NULL, list);
	g_free(tmp);
}

static void
groupmsg_extended_menu_cb(PurpleBlistNode *node, GList **m)
{
	PurpleMenuAction *bna = NULL;

	if(!PURPLE_BLIST_NODE_IS_GROUP(node))
		return;

	*m = g_list_append(*m, bna);
	bna = purple_menu_action_new("Group IM", PURPLE_CALLBACK(groupmsg_sendto_group), NULL, NULL);
	*m = g_list_append(*m, bna);
}

static gboolean
plugin_load(PurplePlugin *plugin)
{

	purple_signal_connect(purple_blist_get_handle(), "blist-node-extended-menu",
						plugin, PURPLE_CALLBACK(groupmsg_extended_menu_cb), NULL);

	return TRUE;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,								/**< magic			*/
	PURPLE_MAJOR_VERSION,								/**< major version	*/
	PURPLE_MINOR_VERSION,								/**< minor version	*/
	PURPLE_PLUGIN_STANDARD,							/**< type			*/
	NULL,											/**< ui_requirement	*/
	0,												/**< flags			*/
	NULL,											/**< dependencies	*/
	PURPLE_PRIORITY_DEFAULT,							/**< priority		*/

	"core-plugin_pack-groupmsg",					/**< id				*/
	NULL,											/**< name			*/
	PP_VERSION,									/**< version		*/
	NULL,											/**  summary		*/
	NULL,											/**  description	*/
	"Stu Tomlinson <stu@nosnilmot.com>",			/**< author			*/
	PP_WEBSITE,									/**< homepage		*/

	plugin_load,									/**< load			*/
	NULL,											/**< unload			*/
	NULL,											/**< destroy		*/

	NULL,											/**< ui_info		*/
	NULL,											/**< extra_info		*/
	NULL,											/**< prefs_info		*/
	NULL											/**< actions		*/
};


static void
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Group IM");
	info.summary = _("Send an IM to a group of buddies.");
	info.description = _("Adds the option to send an IM to every online "
						 "buddy in a group.");
}

PURPLE_INIT_PLUGIN(groupmsg, init_plugin, info)

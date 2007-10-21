/*
 * Show Offline - Show specific buddies while offline
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301, USA.
 */

#include "../common/pp_internal.h"

#include <debug.h>
#include <notify.h>
#include <request.h>
#include <signals.h>
#include <util.h>

static void
showoffline_cb(PurpleBlistNode *node, gpointer data)
{
	PurpleBuddyList *blist = purple_get_blist();
	PurpleBlistUiOps *ops = purple_blist_get_ui_ops();

	if (PURPLE_BLIST_NODE_IS_BUDDY(node))
	{
		purple_blist_node_set_bool(node, "show_offline",
								 !purple_blist_node_get_bool(node, "show_offline"));
	}
	else if (PURPLE_BLIST_NODE_IS_CONTACT(node))
	{
		PurpleBlistNode *bnode;
		gboolean setting = !purple_blist_node_get_bool(node, "show_offline");

		purple_blist_node_set_bool(node, "show_offline", setting);
		for (bnode = node->child; bnode != NULL; bnode = bnode->next) {
			purple_blist_node_set_bool(bnode, "show_offline", setting);
		}
	}
	else
	{
		g_return_if_reached();
	}

	ops->update(blist, node);
}

static void
showoffline_extended_menu_cb(PurpleBlistNode *node, GList **m)
{
	PurpleMenuAction *bna = NULL;

	if (purple_blist_node_get_flags(node) & PURPLE_BLIST_NODE_FLAG_NO_SAVE)
		return;

	if (!PURPLE_BLIST_NODE_IS_CONTACT(node) && !PURPLE_BLIST_NODE_IS_BUDDY(node))
		return;

	*m = g_list_append(*m, bna);

	if (purple_blist_node_get_bool(node, "show_offline"))
		bna = purple_menu_action_new(_("Hide when offline"), PURPLE_CALLBACK(showoffline_cb),
										NULL, NULL);
	else
		bna = purple_menu_action_new(_("Show when offline"), PURPLE_CALLBACK(showoffline_cb),
										NULL, NULL);

	*m = g_list_append(*m, bna);
}

static gboolean
plugin_load(PurplePlugin *plugin)
{

	purple_signal_connect(purple_blist_get_handle(), "blist-node-extended-menu",
						plugin, PURPLE_CALLBACK(showoffline_extended_menu_cb), NULL);

	return TRUE;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,							/**< major version	*/
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,							/**< type			*/
	NULL,											/**< ui_requirement	*/
	0,												/**< flags			*/
	NULL,											/**< dependencies	*/
	PURPLE_PRIORITY_DEFAULT,						/**< priority		*/

	"core-plugin_pack-showoffline",					/**< id				*/
	NULL,											/**< name			*/
	PP_VERSION,										/**< version		*/
	NULL,											/**  summary		*/
	NULL,											/**  description	*/
	"Stu Tomlinson <stu@nosnilmot.com>",			/**< author			*/
	PP_WEBSITE,										/**< homepage		*/

	plugin_load,									/**< load			*/
	NULL,											/**< unload			*/
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

	info.name = _("Show Offline");
	info.summary = _("Show specific buddies while offline.");
	info.description = _("Adds the option to show specific buddies in your buddy "
						 "list when they are offline, even with \"Show Offline Buddies\" turned off.");
}

PURPLE_INIT_PLUGIN(showoffline, init_plugin, info)

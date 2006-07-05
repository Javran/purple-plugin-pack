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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#if HAVE_CONFIG_H
# include "../gpp_config.h"
#endif /* HAVE_CONFIG_H */

#define GAIM_PLUGINS

#include <debug.h>
#include <notify.h>
#include <request.h>
#include <signals.h>
#include <util.h>
#include <version.h>

#include "../common/i18n.h"

static void
showoffline_cb(GaimBlistNode *node, gpointer data)
{
	GaimBuddyList *blist = gaim_get_blist();
	GaimBlistUiOps *ops = gaim_blist_get_ui_ops();
	gaim_blist_node_set_bool(node, "show_offline",
							 !gaim_blist_node_get_bool(node, "show_offline"));
	ops->update(blist, node);
}

static void
showoffline_extended_menu_cb(GaimBlistNode *node, GList **m)
{
	GaimMenuAction *bna = NULL;

	if (!GAIM_BLIST_NODE_IS_BUDDY(node))
		return;

	*m = g_list_append(*m, bna);

	if (gaim_blist_node_get_bool(node, "show_offline"))
		bna = gaim_menu_action_new(_("Hide when offline"), GAIM_CALLBACK(showoffline_cb),
										NULL, NULL);
	else
		bna = gaim_menu_action_new(_("Show when offline"), GAIM_CALLBACK(showoffline_cb),
										NULL, NULL);

	*m = g_list_append(*m, bna);
}

static gboolean
plugin_load(GaimPlugin *plugin)
{

	gaim_signal_connect(gaim_blist_get_handle(), "blist-node-extended-menu",
						plugin, GAIM_CALLBACK(showoffline_extended_menu_cb), NULL);

	return TRUE;
}

static GaimPluginInfo info =
{
	GAIM_PLUGIN_MAGIC,
	GAIM_MAJOR_VERSION,								/**< major version	*/
	GAIM_MINOR_VERSION,
	GAIM_PLUGIN_STANDARD,							/**< type			*/
	NULL,											/**< ui_requirement	*/
	0,												/**< flags			*/
	NULL,											/**< dependencies	*/
	GAIM_PRIORITY_DEFAULT,							/**< priority		*/

	"core-plugin_pack-showoffline",					/**< id				*/
	NULL,											/**< name			*/
	VERSION,										/**< version		*/
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
	bindtextdomain(GPP_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GPP_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Show Offline");
	info.summary = _("Show specific buddies while offline.");
	info.description = _("Adds the option to show specific buddies in your buddy "
						 "list when they are offline, even with \"Show Offline Buddies\" turned off.");
}

GAIM_INIT_PLUGIN(showoffline, init_plugin, info)

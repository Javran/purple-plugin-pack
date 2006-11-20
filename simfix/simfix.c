/*
 * simfix - fix received messages from SIM clients in Gaim
 *
 * (C) Copyright 2005 Stu Tomlinson <stu@nosnilmot.com>
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

#include <plugin.h>
#include <debug.h>
#include <blist.h>
#include <signals.h>
#include <util.h>
#include <version.h>

#include "../common/i18n.h"

static gboolean
receiving_im_msg_cb(GaimAccount *account, char **sender, char **message,
				   GaimConversation *conv, int *flags, void *data)
{
	GaimBuddy *buddy;
	GSList *buds;
	gboolean simuser = FALSE;

	for (buds = gaim_find_buddies(account, *sender); buds; buds = buds->next)
	{
		buddy = buds->data;
		if (gaim_blist_node_get_bool((GaimBlistNode*)buddy, "sim-user"))
			simuser = TRUE;
	}

	if (simuser)
	{
		char *tmp, *stripped;
		tmp = gaim_unescape_html(*message);
		stripped = gaim_markup_strip_html(tmp);
		g_free(tmp);
		g_free(*message);
		*message = stripped;
	}

	return FALSE;
}
static void
simfix_set_simuser(GaimBlistNode *node, gpointer data)
{
	gboolean simuser = GPOINTER_TO_INT(data);
	gaim_blist_node_set_bool(node, "sim-user", simuser);
}

static void
simfix_extended_menu_cb(GaimBlistNode *node, GList **m)
{
	GaimMenuAction *bna = NULL;

	if(!GAIM_BLIST_NODE_IS_BUDDY(node))
		return;
	if (!gaim_blist_node_get_bool(node, "sim-user"))
		bna = gaim_menu_action_new("Set SIM user", GAIM_CALLBACK(simfix_set_simuser),
										GINT_TO_POINTER(TRUE), NULL);
	else
		bna = gaim_menu_action_new("Unset SIM user", GAIM_CALLBACK(simfix_set_simuser),
										GINT_TO_POINTER(FALSE), NULL);
	*m = g_list_append(*m, bna);
}

static gboolean
plugin_load(GaimPlugin *plugin)
{
	gaim_signal_connect(gaim_conversations_get_handle(), "receiving-im-msg",
						plugin, GAIM_CALLBACK(receiving_im_msg_cb), NULL);
	gaim_signal_connect(gaim_blist_get_handle(), "blist-node-extended-menu",
						plugin, GAIM_CALLBACK(simfix_extended_menu_cb), NULL);

	return TRUE;
}

static GaimPluginInfo info =
{
	GAIM_PLUGIN_MAGIC,								/**< magic			*/
	GAIM_MAJOR_VERSION,								/**< major version	*/
	GAIM_MINOR_VERSION,								/**< minor version	*/
	GAIM_PLUGIN_STANDARD,							/**< type			*/
	GAIM_PLUGIN_STANDARD,							/**< ui_requirement	*/
	0,												/**< flags			*/
	NULL,											/**< dependencies	*/
	GAIM_PRIORITY_DEFAULT,							/**< priority		*/

	"core-plugin_pack-simfix",						/**< id				*/
	N_("SIM-fix"),									/**< name			*/
	GPP_VERSION,									/**< version		*/
													/**  summary		*/
	N_("Fix messages from broken SIM clients."),
													/**  description	*/
	N_("Fixes messages received from broken SIM clients by "
	   "stripping HTML from them. The buddy must be on your "
	   "list and set as a SIM user."),
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
init_plugin(GaimPlugin *plugin)
{
}

GAIM_INIT_PLUGIN(simfix, init_plugin, info)

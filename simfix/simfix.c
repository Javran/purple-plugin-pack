/*
 * simfix - fix received messages from SIM clients in Purple
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
# include "../pp_config.h"
#endif /* HAVE_CONFIG_H */

#define PURPLE_PLUGINS

#include <plugin.h>
#include <debug.h>
#include <blist.h>
#include <signals.h>
#include <util.h>
#include <version.h>

#include "../common/pp_internal.h"

static gboolean
receiving_im_msg_cb(PurpleAccount *account, char **sender, char **message,
				   PurpleConversation *conv, int *flags, void *data)
{
	PurpleBuddy *buddy;
	GSList *buds;
	gboolean simuser = FALSE;

	for (buds = purple_find_buddies(account, *sender); buds; buds = buds->next)
	{
		buddy = buds->data;
		if (purple_blist_node_get_bool((PurpleBlistNode*)buddy, "sim-user"))
			simuser = TRUE;
	}

	if (simuser)
	{
		char *tmp, *stripped;
		tmp = purple_unescape_html(*message);
		stripped = purple_markup_strip_html(tmp);
		g_free(tmp);
		g_free(*message);
		*message = stripped;
	}

	return FALSE;
}
static void
simfix_set_simuser(PurpleBlistNode *node, gpointer data)
{
	gboolean simuser = GPOINTER_TO_INT(data);
	purple_blist_node_set_bool(node, "sim-user", simuser);
}

static void
simfix_extended_menu_cb(PurpleBlistNode *node, GList **m)
{
	PurpleMenuAction *bna = NULL;

	if (purple_blist_node_get_flags(node) & PURPLE_BLIST_NODE_FLAG_NO_SAVE)
		return;

	if(!PURPLE_BLIST_NODE_IS_BUDDY(node))
		return;
	if (!purple_blist_node_get_bool(node, "sim-user"))
		bna = purple_menu_action_new("Set SIM user", PURPLE_CALLBACK(simfix_set_simuser),
										GINT_TO_POINTER(TRUE), NULL);
	else
		bna = purple_menu_action_new("Unset SIM user", PURPLE_CALLBACK(simfix_set_simuser),
										GINT_TO_POINTER(FALSE), NULL);
	*m = g_list_append(*m, bna);
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	purple_signal_connect(purple_conversations_get_handle(), "receiving-im-msg",
						plugin, PURPLE_CALLBACK(receiving_im_msg_cb), NULL);
	purple_signal_connect(purple_blist_get_handle(), "blist-node-extended-menu",
						plugin, PURPLE_CALLBACK(simfix_extended_menu_cb), NULL);

	return TRUE;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,							/**< magic			*/
	PURPLE_MAJOR_VERSION,							/**< major version	*/
	PURPLE_MINOR_VERSION,							/**< minor version	*/
	PURPLE_PLUGIN_STANDARD,							/**< type			*/
	PURPLE_PLUGIN_STANDARD,							/**< ui_requirement	*/
	0,												/**< flags			*/
	NULL,											/**< dependencies	*/
	PURPLE_PRIORITY_DEFAULT,						/**< priority		*/

	"core-plugin_pack-simfix",						/**< id				*/
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
#endif

	info.name = _("SIM-fix");
	info.summary = _("Fix messages from broken SIM clients.");
	info.description = _("Fixes messages received from broken SIM clients by "
						 "stripping HTML from them. The buddy must be on your "
						 "list and set as a SIM user.");
}

PURPLE_INIT_PLUGIN(simfix, init_plugin, info)

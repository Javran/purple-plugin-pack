/*
 * Purple Plugin Pack
 * Copyright (C) 2003-2005
 * See ../AUTHORS for a list of all authors
 *
 * listhandler: Provides importing, exporting, and copying functions
 * 				for accounts' buddy lists.
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

#include "listhandler.h"
#include "aim_blt_files.h"
#include "alias_xml_files.h"
#include "gen_xml_files.h"
#include "migrate.h"

PurplePlugin *listhandler = NULL; /* the request api prefers this for a plugin */

static GList * /* libpurple's picky and wants a GList of actions for the UI menu */
listhandler_actions(PurplePlugin *plugin, gpointer context)
{
	GList *list = NULL;
	PurplePluginAction *action = NULL;

	action = purple_plugin_action_new(_("Copy Buddies From One Account to Another"),
									lh_migrate_action_cb);
	list = g_list_append(list, action);

	action = purple_plugin_action_new(_("Import Alias List File"),
									lh_alist_import_action_cb);
	list = g_list_append(list, action);

	action = purple_plugin_action_new(_("Import AIM Buddy List File (.blt)"),
									lh_aim_import_action_cb);
	list = g_list_append(list, action);

	action = purple_plugin_action_new(_("Import Generic Buddy List File (.xml)"),
									lh_generic_import_action_cb);
	list = g_list_append(list, action);

	action = purple_plugin_action_new(_("Export AIM Buddy List File"),
									lh_aim_export_action_cb);
	list = g_list_append(list, action);

	action = purple_plugin_action_new(_("Export Alias List File"),
									lh_alist_export_action_cb);
	list = g_list_append(list, action);

	action = purple_plugin_action_new(_("Export Generic Buddy List File"),
									lh_generic_export_action_cb);
	list = g_list_append(list, action);

	purple_debug_info("listhandler", "Action list created\n");

	return list;
}

static PurplePluginInfo listhandler_info =
{
	PURPLE_PLUGIN_MAGIC, /* abracadabra */
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,
	"core-plugin_pack-listhandler",
	NULL,
	PP_VERSION,
	NULL,
	NULL,
	"John Bailey <rekkanoryo@rekkanoryo.org>",
	PP_WEBSITE,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	listhandler_actions,
	NULL,
	NULL,
	NULL,
	NULL
};

static void /* purple will call this to initialize the plugin */
init_plugin(PurplePlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	listhandler_info.name = _("List Handler");
	listhandler_info.summary =
					_("Provides numerous user-requested list-handling "
					"capabilities.");
	listhandler_info.description =
					_("Provides numerous user-requested list-handling "
					"capabilities, such as importing and exporting of AIM "
					".blt files and generic protocol-agnostic XML .blist "
					"files, as well as direct copying of buddies from one "
					"account to another.");

	listhandler = plugin; /* handle needed for request API file selector */

	return;
}

PURPLE_INIT_PLUGIN(listhandler, init_plugin, listhandler_info)

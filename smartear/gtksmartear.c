/*
 * gktsmartear.c - GTK+ configuration UI plugin to accompany smartear.
 * Copyright (C) 2007 John Bailey <rekkanoryo@rekkanoryo.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
# include "../pp_config.h"
#endif

/* Pack/Local headers */
#include "../common/pp_internal.h"

/* System headers */
#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>

/* Pidgin headers */
#include <gtkplugin.h>

static void
gtksmartear_blist_menu_cb(PurpleBlistNode *node, gpointer data) {
}

static void
gtksmartear_drawing_blist_menu_cb(PurpleBlistNode *node, GList **menu) {
	/* Don't do anything if the blistnode won't be saved */
	if(purple_blist_node_get_flags(node) & PURPLE_BLIST_NODE_FLAG_NO_SAVE)
		return;

	/* We don't support setting anything for a chat, since there's no way
	 * to get the name of a chat */
	if(PURPLE_BLIST_NODE_IS_CHAT(node))
		return;

	(*menu) = g_list_append(*menu, purple_menu_action_new(_("SmartEar Options"),
							PURPLE_CALLBACK(gtksmartear_blist_menu_cb),
							NULL, NULL));
}

static gboolean
plugin_load(PurplePlugin *plugin) {
	purple_signal_connect(purple_blist_get_handle(),
						"blist-node-extended-menu",
						plugin,
						PURPLE_CALLBACK(gtksmartear_drawing_blist_menu_cb),
						NULL);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin) {
	return TRUE;
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,		/* Magic				*/
	PURPLE_MAJOR_VERSION,		/* Purple Major Version	*/
	PURPLE_MINOR_VERSION,		/* Purple Minor Version	*/
	PURPLE_PLUGIN_STANDARD,		/* plugin type			*/
	PIDGIN_PLUGIN_TYPE,			/* ui requirement		*/
	0,							/* flags				*/
	NULL,						/* dependencies			*/
	PURPLE_PRIORITY_DEFAULT,	/* priority				*/

	"gtk-plugin_pack-smartear",	/* plugin id			*/
	NULL,						/* name					*/
	PP_VERSION,					/* version				*/
	NULL,						/* summary				*/
	NULL,						/* description			*/
	"John Bailey <rekkanoryo@rekkanoryo.org>",
	PP_WEBSITE,					/* website				*/

	plugin_load,				/* load					*/
	plugin_unload,				/* unload				*/
	NULL,						/* destroy				*/

	NULL,						/* ui_info				*/
	NULL,						/* extra_info			*/
	NULL,						/* prefs_info			*/
	NULL,						/* actions				*/

	NULL,						/* reserved 1			*/
	NULL,						/* reserved 2			*/
	NULL,						/* reserved 3			*/
	NULL						/* reserved 4			*/
};

static void
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("SmartEar");
	info.summary = _("The GTK+ (Pidgin) component of the SmartEar plugin suite");
	info.description = _("This plugin provides the Pidgin interface to the "
						"SmartEar plugin suite's functionality.  The suite "
						"allows you to specify sounds per-buddy, per-contact, "
						"or per-group for specific events.");

	info.dependencies = g_list_append(NULL, "core-plugin_pack-smartear");
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info)

/*
 * Hides the blist on signon (or when it's created)
 * Copyright (C) 2004 Gary Kramlich.
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
#if HAVE_CONFIG_H
# include "../gpp_config.h"
#endif

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#define GAIM_PLUGINS

#include <gtkplugin.h>
#include <gtkblist.h>
#include <pluginpref.h>
#include <prefs.h>
#include <version.h>

#include "../common/i18n.h"

#define PREF_MY "/plugins/gtk/amc_grim"
#define PREF_ROOT "/plugins/gtk/amc_grim/blistops"
#define PREF_LIST "/plugins/gtk/amc_grim/blistops/hidelist"
#define PREF_MENU "/plugins/gtk/amc_grim/blistops/hidemenu"

static GtkWidget *w_blist = NULL;
static GtkWidget *w_menubar = NULL;

static void
abracadabra(GtkWidget *w, gboolean v) {
	if(v)
		gtk_widget_hide(w);
	else
		gtk_widget_show(w);
}

static void
pref_cb(const char *name, GaimPrefType type,
		gconstpointer val, gpointer data) {
	gboolean value = GPOINTER_TO_INT(val);

	if(!g_ascii_strcasecmp(name, PREF_LIST))
		abracadabra(w_blist, value);
	else if(!g_ascii_strcasecmp(name, PREF_MENU))
		abracadabra(w_menubar, value);
}

static void
gtkblist_created_cb(GaimBuddyList *blist) {
	GaimGtkBuddyList *gtkblist = GAIM_GTK_BLIST(blist);

	w_blist = gtkblist->window;
	w_menubar = gtk_item_factory_get_widget(gtkblist->ift, "<GaimMain>");
}

/* globals to remove our pref cb's */
guint blist_id = 0, menu_id = 0;

static gboolean
plugin_load(GaimPlugin *plugin) {
	gaim_signal_connect(gaim_gtk_blist_get_handle(), "gtkblist-created", plugin,
						GAIM_CALLBACK(gtkblist_created_cb), NULL);

	blist_id = gaim_prefs_connect_callback(plugin, PREF_LIST, pref_cb, NULL);
	menu_id = gaim_prefs_connect_callback(plugin, PREF_MENU, pref_cb, NULL);

	return TRUE;
}

static gboolean
plugin_unload(GaimPlugin *plugin) {
	gaim_prefs_disconnect_callback(blist_id);
	gaim_prefs_disconnect_callback(menu_id);

	if(w_blist) {
		gtk_widget_show(w_blist);

		if(w_menubar)
			gtk_widget_show(w_menubar);
	}

	return TRUE;
}

static GaimPluginPrefFrame *
get_plugin_pref_frame(GaimPlugin *plugin) {
	GaimPluginPrefFrame *frame;
	GaimPluginPref *ppref;

	frame = gaim_plugin_pref_frame_new();

	ppref = gaim_plugin_pref_new_with_name_and_label(PREF_LIST,									
									_("Hide the buddy list when it is created"));
	gaim_plugin_pref_frame_add(frame, ppref);

	ppref = gaim_plugin_pref_new_with_name_and_label(PREF_MENU,
									_("Hide the menu in the buddy list window"));
	gaim_plugin_pref_frame_add(frame, ppref);

	return frame;
}

static GaimPluginUiInfo prefs_info = {
	get_plugin_pref_frame
};

static GaimPluginInfo info =
{
	GAIM_PLUGIN_MAGIC,
	GAIM_MAJOR_VERSION,
	GAIM_MINOR_VERSION,
	GAIM_PLUGIN_STANDARD,
	GAIM_GTK_PLUGIN_TYPE,
	0,
	NULL,
	GAIM_PRIORITY_DEFAULT,

	"gtk-plugin_pack-blistops",
	NULL,
	GPP_VERSION,
	NULL,
	NULL,
	"Gary Kramlich <amc_grim@users.sf.net>",
	GPP_WEBSITE,

	plugin_load,
	plugin_unload,
	NULL,

	NULL,
	NULL,
	&prefs_info,
	NULL
};

static void
init_plugin(GaimPlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Buddy List Options");
	info.summary = _("Gives extended options to the buddy list");
	info.description = _("Gives extended options to the buddy list");

	info.dependencies = g_list_append(info.dependencies, "gtk-docklet");

	gaim_prefs_add_none(PREF_MY);
	gaim_prefs_add_none(PREF_ROOT);
	gaim_prefs_add_bool(PREF_LIST, TRUE);
	gaim_prefs_add_bool(PREF_MENU, TRUE);
}

GAIM_INIT_PLUGIN(blistops, init_plugin, info)

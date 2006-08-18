/*
 * mystatusbox - Show/Hide the peraccount statusboxes
 * Copyright (C) 2005
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

#define GAIM_PLUGINS

#define PLUGIN_ID			"gtk-plugin_pack-mystatusbox"
#define PLUGIN_NAME			"Mystatusbox (Show Statusboxes)"
#define PLUGIN_STATIC_NAME	"mystatusbox"
#define PLUGIN_SUMMARY		"Hide/Show the per-account statusboxes"
#define PLUGIN_DESCRIPTION	"You can show all the per-account statusboxes, " \
							"hide all of them, or just show the ones that are " \
							"in a different status than the global status.\n\n" \
							"For ease of use, you can bind keyboard-shortcuts " \
							"for the menu-items."
#define PLUGIN_AUTHOR		"Sadrul Habib Chowdhury <sadrul@users.sourceforge.net>"

/* System headers */
#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>

/* Gaim headers */
#include <version.h>

#if GAIM_VERSION_CHECK(2,0,0) /* Stu is a chicken */

#include <core.h>

#include <gtkblist.h>
#include <gtkmenutray.h>
#include <gtkplugin.h>
#include <gtkstatusbox.h>

/* Pack/Local headers */
#include "../common/i18n.h"

#define PREF_PREFIX "/gaim/gtk/" PLUGIN_ID
#define PREF_PANE	PREF_PREFIX "/pane"
#define PREF_GLOBAL	PREF_PREFIX "/global"
#define PREF_SHOW	PREF_PREFIX "/show"
#define PREF_ICONS_HIDE   PREF_PREFIX   "/iconsel"

static GtkWidget *gtkblist_statusboxbox;
static GList *gtkblist_statusboxes;

typedef enum
{
	GAIM_STATUSBOX_ALL,
	GAIM_STATUSBOX_NONE,
	GAIM_STATUSBOX_OUT_SYNC
} GaimStatusBoxVisibility;

static void gaim_gtk_status_selectors_show(GaimStatusBoxVisibility action);

static gboolean
pane_position_cb(GtkPaned *paned, GParamSpec *param_spec, gpointer data)
{   
	gaim_prefs_set_int(PREF_PANE, gtk_paned_get_position(paned));
	return FALSE;	
}

static void
account_enabled_cb(GaimAccount *account, gpointer null)
{
	if (gaim_account_get_enabled(account, gaim_core_get_ui()))
	{
		GtkWidget *box = gtk_gaim_status_box_new_with_account(account);
		g_object_set(box, "iconsel", !gaim_prefs_get_bool(PREF_ICONS_HIDE), NULL);
		gtk_widget_set_name(box, "gaim_gtkblist_statusbox_account");
		gtk_box_pack_start(GTK_BOX(gtkblist_statusboxbox), box, FALSE, TRUE, 0);
		gtk_widget_show(box);
		gtkblist_statusboxes = g_list_append(gtkblist_statusboxes, box);
	}
}

static void
account_disabled_cb(GaimAccount *account, gpointer null)
{
	GList *iter;

	for (iter = gtkblist_statusboxes; iter; iter = iter->next)
	{
		GtkGaimStatusBox *box = iter->data;
		if (box->account == account)
		{
			gtkblist_statusboxes = g_list_remove(gtkblist_statusboxes, box);
			gtk_widget_destroy(GTK_WIDGET(box));
			break;
		}
	}
}

static void
update_out_of_sync()
{
	if (gaim_prefs_get_int(PREF_SHOW) == GAIM_STATUSBOX_OUT_SYNC)
		gaim_gtk_status_selectors_show(GAIM_STATUSBOX_OUT_SYNC);
}

static void
detach_per_account_boxes()
{
	GaimGtkBuddyList *gtkblist;
	GList *list, *iter;
	int i;

	gtkblist = gaim_gtk_blist_get_default_gtk_blist();
	if (!gtkblist)
		return;

	GtkWidget *holds[] = {gtkblist->menutray->parent, gtkblist->treeview->parent,
				gtkblist->error_buttons, gtkblist->statusbox, NULL};

	for (i = 0; holds[i]; i++)
	{
		g_object_ref(G_OBJECT(holds[i]));
		gtk_container_remove(GTK_CONTAINER(holds[i]->parent), holds[i]);
	}
	

	list = gtk_container_get_children(GTK_CONTAINER(gtkblist->vbox));
	for (iter = list; iter; iter = iter->next)
		gtk_container_remove(GTK_CONTAINER(gtkblist->vbox), iter->data);

	gtk_box_pack_start(GTK_BOX(gtkblist->vbox), gtkblist->menutray->parent, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(gtkblist->vbox), gtkblist->treeview->parent, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(gtkblist->vbox), gtkblist->error_buttons, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(gtkblist->vbox), gtkblist->statusbox, FALSE, FALSE, 0);


	for (i = 0; holds[i]; i++)
		g_object_unref(G_OBJECT(holds[i]));
	
	gtkblist_statusboxbox = NULL;
}

static void
attach_per_account_boxes()
{
	GaimGtkBuddyList *gtkblist;
	GList *list, *iter;
	GtkWidget *sw2, *vpane, *vbox;

	gtkblist = gaim_gtk_blist_get_default_gtk_blist();

	if (!gtkblist || gtkblist_statusboxbox)
		return;

	gtkblist_statusboxbox = gtk_vbox_new(FALSE, 0);
	gtkblist_statusboxes = NULL;

	list = gaim_accounts_get_all_active();
	for (iter = list; iter; iter = iter->next)
	{
		account_enabled_cb(iter->data, NULL);
	}
	g_list_free(list);

	gtk_widget_show_all(gtkblist_statusboxbox);

	list = gtk_container_get_children(GTK_CONTAINER(gtkblist->vbox));
	for (iter = list; iter; iter = iter->next)
	{
		g_object_ref(iter->data);
		gtk_container_remove(GTK_CONTAINER(gtkblist->vbox), iter->data);
	}

	gtk_box_pack_start(GTK_BOX(gtkblist->vbox), gtkblist->menutray->parent, FALSE, FALSE, 0);
	g_object_unref(G_OBJECT(gtkblist->menutray->parent));

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gtkblist->treeview->parent, TRUE, TRUE, 0);
	g_object_unref(G_OBJECT(gtkblist->treeview->parent));
	gtk_box_pack_start(GTK_BOX(vbox), gtkblist->error_buttons, FALSE, FALSE ,0);
	g_object_unref(G_OBJECT(gtkblist->error_buttons));

	vpane = gtk_vpaned_new();
	gtk_box_pack_start(GTK_BOX(gtkblist->vbox), vpane, TRUE, TRUE, 0);

	gtk_paned_pack1(GTK_PANED(vpane), vbox, TRUE, FALSE);

	sw2 = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw2), gtkblist_statusboxbox);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw2), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_show(sw2);
	gtk_paned_pack2(GTK_PANED(vpane), sw2, FALSE, TRUE);
	gtk_widget_show_all(vpane);

	gtk_box_pack_start(GTK_BOX(gtkblist->vbox), gtkblist->statusbox, FALSE, TRUE, 0);
	g_object_unref(G_OBJECT(gtkblist->statusbox));
	if (gaim_prefs_get_bool(PREF_GLOBAL))
		gtk_widget_hide(gtkblist->statusbox);
	else
		gtk_widget_show(gtkblist->statusbox);
	
	g_object_set(gtkblist->statusbox, "iconsel", !gaim_prefs_get_bool(PREF_ICONS_HIDE), NULL);

	gaim_gtk_status_selectors_show(gaim_prefs_get_int(PREF_SHOW));

	gtk_paned_set_position(GTK_PANED(vpane),																				 gaim_prefs_get_int(PREF_PANE));
	g_signal_connect(G_OBJECT(vpane), "notify::position",
							G_CALLBACK(pane_position_cb), NULL);
}

static void
gaim_gtk_status_selectors_show(GaimStatusBoxVisibility action)
{
	GtkRequisition req;
	int height;
	GList *list;
	GaimGtkBuddyList *gtkblist;

	gtkblist = gaim_gtk_blist_get_default_gtk_blist();

	gaim_prefs_set_int(PREF_SHOW, action);

	if (!gtkblist || !gtkblist_statusboxbox)
		return;

	height = gaim_prefs_get_int("/gaim/gtk/blist/height");
	
	if (!gaim_prefs_get_bool(PREF_GLOBAL))
	{
		gtk_widget_size_request(gtkblist->statusbox, &req);
		height -= req.height;	/* Height of the global statusbox */
	}

	list = gtkblist_statusboxes;
	for (; list; list = list->next)
	{
		GtkWidget *box = list->data;

		if (action == GAIM_STATUSBOX_ALL)	/* Show all */
		{
			gtk_widget_show_all(box);
			gtk_widget_size_request(box, &req);
			height -= req.height;
		}
		else if (action == GAIM_STATUSBOX_NONE)	/* Show none */
		{
			gtk_widget_hide_all(box);
		}
		else if (action == GAIM_STATUSBOX_OUT_SYNC)	/* Show non-synced ones */
		{
			GaimAccount *account = GTK_GAIM_STATUS_BOX(box)->account;
			GaimStatus *status;
			GaimStatusPrimitive account_prim, global_prim;
			GaimSavedStatus *saved;
			GaimSavedStatusSub *sub;
			gboolean show = TRUE;
			const char *global_message;

			/* This, unfortunately, is necessary, until (if at all) #1440568 gets in */
			if (!gaim_account_is_connected(account))
				status = gaim_account_get_status(account, "offline");
			else
				status = gaim_account_get_active_status(account);

			account_prim = gaim_status_type_get_primitive(gaim_status_get_type(status));

			saved = gaim_savedstatus_get_current();
			sub = gaim_savedstatus_get_substatus(saved, account);
			if (sub)
			{
				global_prim = gaim_status_type_get_primitive(gaim_savedstatus_substatus_get_type(sub));
				global_message = gaim_savedstatus_substatus_get_message(sub);
			}
			else
			{
				global_prim = gaim_savedstatus_get_type(saved);
				global_message = gaim_savedstatus_get_message(saved);
			}

			if (global_prim == account_prim)
			{
				if (gaim_status_type_get_attr(gaim_status_get_type(status), "message"))
				{
					const char *message = NULL;

					message = gaim_status_get_attr_string(status, "message");

					if ((global_message == NULL && message == NULL) ||
							(global_message && message && g_utf8_collate(global_message, message) == 0))
						show = FALSE;
				}
				else
					show = FALSE;
			}

			if (show)
			{
				gtk_widget_show_all(box);
				gtk_widget_size_request(box, &req);
				height -= req.height;
			}
			else
				gtk_widget_hide_all(box);
		}
	}

	gtk_widget_size_request(gtkblist->menutray->parent, &req);
	height -= req.height;	/* Height of the menu */

	height -= 9;	/* Height of the handle of the vpane */

	gtk_paned_set_position(GTK_PANED(gtkblist_statusboxbox->parent->parent->parent), height);
}

static void
show_boxes_all(GaimPluginAction *action)
{
	gaim_gtk_status_selectors_show(GAIM_STATUSBOX_ALL);
}

static void
show_boxes_none(GaimPluginAction *action)
{
	gaim_gtk_status_selectors_show(GAIM_STATUSBOX_NONE);
}

static void
show_boxes_out_of_sync(GaimPluginAction *action)
{
	gaim_gtk_status_selectors_show(GAIM_STATUSBOX_OUT_SYNC);
}

static void
toggle_icons(GaimPluginAction *action)
{
	gboolean v = gaim_prefs_get_bool(PREF_ICONS_HIDE);
	gaim_prefs_set_bool(PREF_ICONS_HIDE, !v);
}

static void
toggle_global(GaimPluginAction *action)
{
	gboolean v = gaim_prefs_get_bool(PREF_GLOBAL);
	gaim_prefs_set_bool(PREF_GLOBAL, !v);
}

static GList *
actions(GaimPlugin *plugin, gpointer context)
{
	GList *l = NULL;
	GaimPluginAction *act = NULL;

	act = gaim_plugin_action_new(_("All"), show_boxes_all);
	l = g_list_append(l, act);

	act = gaim_plugin_action_new(_("None"), show_boxes_none);
	l = g_list_append(l, act);

	act = gaim_plugin_action_new(_("Out of sync ones"), show_boxes_out_of_sync);
	l = g_list_append(l, act);

	l = g_list_append(l, NULL);
	
	act = gaim_plugin_action_new(_("Toggle icon selectors"), toggle_icons);
	l = g_list_append(l, act);

	act = gaim_plugin_action_new(_("Toggle global selector"), toggle_global);
	l = g_list_append(l, act);

	return l;
}

static void
toggle_iconsel_cb(const char *name, GaimPrefType type, gconstpointer val, gpointer null)
{
	GList *iter = gtkblist_statusboxes;
	gboolean value = !GPOINTER_TO_INT(val);
	GaimGtkBuddyList *gtkblist;

	for (; iter; iter = iter->next)
	{
		g_object_set(iter->data, "iconsel", value, NULL);
	}

	gtkblist = gaim_gtk_blist_get_default_gtk_blist();
	if (gtkblist)
		g_object_set(gtkblist->statusbox, "iconsel", value, NULL);
}

static void
hide_global_callback(const char *name, GaimPrefType type, gconstpointer val, gpointer null)
{
	GaimGtkBuddyList *gtkblist = gaim_gtk_blist_get_default_gtk_blist();
	gboolean hide = GPOINTER_TO_UINT(val);

	if (!gtkblist)
		return;

	if (hide)
		gtk_widget_hide(gtkblist->statusbox);
	else
		gtk_widget_show(gtkblist->statusbox);
}

static void
account_status_changed_cb(GaimAccount *account, GaimStatus *os, GaimStatus *ns, gpointer null)
{
	update_out_of_sync();
}

static void
global_status_changed_cb(const char *name, GaimPrefType type, gconstpointer val, gpointer null)
{
	update_out_of_sync();
}

static void
signed_on_off_cb(GaimConnection *gc, gpointer null)
{
	update_out_of_sync();
}

static gboolean
plugin_load(GaimPlugin *plugin)
{
	attach_per_account_boxes();
	
	gaim_signal_connect(gaim_gtk_blist_get_handle(), "gtkblist-created",
				plugin, GAIM_CALLBACK(attach_per_account_boxes), NULL);
	gaim_signal_connect(gaim_accounts_get_handle(), "account-added", plugin,
				GAIM_CALLBACK(account_enabled_cb), NULL);
	gaim_signal_connect(gaim_accounts_get_handle(), "account-enabled", plugin,
				GAIM_CALLBACK(account_enabled_cb), NULL);
	gaim_signal_connect(gaim_accounts_get_handle(), "account-removed", plugin,
				GAIM_CALLBACK(account_disabled_cb), NULL);
	gaim_signal_connect(gaim_accounts_get_handle(), "account-disabled", plugin,
				GAIM_CALLBACK(account_disabled_cb), NULL);
	gaim_signal_connect(gaim_accounts_get_handle(), "account-status-changed", plugin,
				GAIM_CALLBACK(account_status_changed_cb), NULL);
	gaim_signal_connect(gaim_connections_get_handle(), "signed-on", plugin,
				GAIM_CALLBACK(signed_on_off_cb), NULL);
	gaim_signal_connect(gaim_connections_get_handle(), "signed-off", plugin,
				GAIM_CALLBACK(signed_on_off_cb), NULL);
		
	gaim_prefs_connect_callback(plugin, PREF_GLOBAL, hide_global_callback, NULL);
	gaim_prefs_connect_callback(plugin, PREF_ICONS_HIDE, toggle_iconsel_cb, NULL);
	gaim_prefs_connect_callback(plugin, "/core/savedstatus/current", global_status_changed_cb, NULL);
	return TRUE;
}

static gboolean
plugin_unload(GaimPlugin *plugin)
{
	GaimGtkBuddyList *gtkblist;
	
	gaim_gtk_status_selectors_show(GAIM_STATUSBOX_ALL);
	detach_per_account_boxes();

	gtkblist = gaim_gtk_blist_get_default_gtk_blist();
	if (gtkblist)
		gtk_widget_show(gtkblist->statusbox);
	gaim_prefs_disconnect_by_handle(plugin);
	
	return TRUE;
}

static GaimPluginPrefFrame *
get_plugin_pref_frame(GaimPlugin *plugin)
{
	GaimPluginPrefFrame *frame;
	GaimPluginPref *pref;

	frame = gaim_plugin_pref_frame_new();

	pref = gaim_plugin_pref_new_with_name_and_label(PREF_GLOBAL, _("Hide global status selector"));
	gaim_plugin_pref_frame_add(frame, pref);

	pref = gaim_plugin_pref_new_with_name_and_label(PREF_ICONS_HIDE, _("Hide icon-selectors"));
	gaim_plugin_pref_frame_add(frame, pref);

	return frame;
}

static GaimPluginUiInfo prefs_info = {
	get_plugin_pref_frame
};

static GaimPluginInfo info =
{
	GAIM_PLUGIN_MAGIC,			/* Magic				*/
	GAIM_MAJOR_VERSION,			/* Gaim Major Version	*/
	GAIM_MINOR_VERSION,			/* Gaim Minor Version	*/
	GAIM_PLUGIN_STANDARD,		/* plugin type			*/
	GAIM_GTK_PLUGIN_TYPE,		/* ui requirement		*/
	0,							/* flags				*/
	NULL,						/* dependencies			*/
	GAIM_PRIORITY_DEFAULT,		/* priority				*/

	PLUGIN_ID,					/* plugin id			*/
	NULL,						/* name					*/
	GPP_VERSION,				/* version				*/
	NULL,						/* summary				*/
	NULL,						/* description			*/
	PLUGIN_AUTHOR,				/* author				*/
	GPP_WEBSITE,				/* website				*/

	plugin_load,				/* load					*/
	plugin_unload,				/* unload				*/
	NULL,						/* destroy				*/

	NULL,						/* ui_info				*/
	NULL,						/* extra_info			*/
	&prefs_info,				/* prefs_info			*/
	actions						/* actions				*/
};

static void
init_plugin(GaimPlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GPP_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GPP_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _(PLUGIN_NAME);
	info.summary = _(PLUGIN_SUMMARY);
	info.description = _(PLUGIN_DESCRIPTION);

	gaim_prefs_add_none(PREF_PREFIX);
	gaim_prefs_add_int(PREF_PANE, 300);
	gaim_prefs_add_bool(PREF_GLOBAL, FALSE);
	gaim_prefs_add_int(PREF_SHOW, GAIM_STATUSBOX_ALL);
	gaim_prefs_add_bool(PREF_ICONS_HIDE, FALSE);
}

GAIM_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info)
#endif

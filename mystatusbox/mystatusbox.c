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
#ifdef HAVE_CONFIG_H
# include "../pp_config.h"
#endif

#define PURPLE_PLUGINS

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

/* Pack/Local headers */
#include "../common/pp_internal.h"

/* Purple headers */
#include <version.h>

#if PURPLE_VERSION_CHECK(2,0,0) /* Stu is a chicken */

#include <core.h>

#include <gtkblist.h>
#include <gtkmenutray.h>
#include <gtkplugin.h>
#include <gtkstatusbox.h>

/* XXX: THIS NEEDS CHANGED WHEN PIDGIN DOES ITS PREFS MIGRATION!!!!! */
#define PREF_PREFIX "/plugins/gtk/" PLUGIN_ID
#define PREF_PANE	PREF_PREFIX "/pane"
#define PREF_GLOBAL	PREF_PREFIX "/global"
#define PREF_SHOW	PREF_PREFIX "/show"
#define PREF_ICONS_HIDE   PREF_PREFIX   "/iconsel"

static GtkWidget *gtkblist_statusboxbox;
static GList *gtkblist_statusboxes;

typedef enum
{
	PURPLE_STATUSBOX_ALL,
	PURPLE_STATUSBOX_NONE,
	PURPLE_STATUSBOX_OUT_SYNC
} PurpleStatusBoxVisibility;

static void pidgin_status_selectors_show(PurpleStatusBoxVisibility action);

static gboolean
pane_position_cb(GtkPaned *paned, GParamSpec *param_spec, gpointer data)
{   
	purple_prefs_set_int(PREF_PANE, gtk_paned_get_position(paned));
	return FALSE;	
}

static void
account_enabled_cb(PurpleAccount *account, gpointer null)
{
	if (purple_account_get_enabled(account, purple_core_get_ui()))
	{
		GtkWidget *box = pidgin_status_box_new_with_account(account);
		g_object_set(box, "iconsel", !purple_prefs_get_bool(PREF_ICONS_HIDE), NULL);
		gtk_widget_set_name(box, "pidginblist_statusbox_account");
		gtk_box_pack_start(GTK_BOX(gtkblist_statusboxbox), box, FALSE, TRUE, 0);
		gtk_widget_show(box);
		gtkblist_statusboxes = g_list_append(gtkblist_statusboxes, box);
	}
}

static void
account_disabled_cb(PurpleAccount *account, gpointer null)
{
	GList *iter;

	for (iter = gtkblist_statusboxes; iter; iter = iter->next)
	{
		PidginStatusBox *box = iter->data;
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
	if (purple_prefs_get_int(PREF_SHOW) == PURPLE_STATUSBOX_OUT_SYNC)
		pidgin_status_selectors_show(PURPLE_STATUSBOX_OUT_SYNC);
}

static void
detach_per_account_boxes()
{
	PidginBuddyList *gtkblist;
	GList *list, *iter;
	int i;
	gboolean headline_showing;
	GtkWidget **holds;

	gtkblist = pidgin_blist_get_default_gtk_blist();
	if (!gtkblist)
		return;

	holds = (GtkWidget*[]){gtkblist->headline_hbox->parent, gtkblist->treeview->parent,
				gtkblist->error_buttons, gtkblist->statusbox,
				gtkblist->scrollbook, NULL};

	headline_showing = GTK_WIDGET_VISIBLE(gtkblist->headline_hbox) && GTK_WIDGET_DRAWABLE(gtkblist->headline_hbox);

	for (i = 0; holds[i]; i++)
	{
		g_object_ref(G_OBJECT(holds[i]));
		gtk_container_remove(GTK_CONTAINER(holds[i]->parent), holds[i]);
	}

	list = gtk_container_get_children(GTK_CONTAINER(gtkblist->vbox));
	for (iter = list; iter; iter = iter->next)
		gtk_container_remove(GTK_CONTAINER(gtkblist->vbox), iter->data);


	gtk_box_pack_start(GTK_BOX(gtkblist->vbox), gtkblist->headline_hbox->parent, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(gtkblist->vbox), gtkblist->treeview->parent, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(gtkblist->vbox), gtkblist->scrollbook, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(gtkblist->vbox), gtk_hseparator_new(), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(gtkblist->vbox), gtkblist->error_buttons, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(gtkblist->vbox), gtkblist->statusbox, FALSE, FALSE, 0);

	if (!headline_showing)
		gtk_widget_hide(gtkblist->headline_hbox);

	for (i = 0; holds[i]; i++)
		g_object_unref(G_OBJECT(holds[i]));
	
	gtkblist_statusboxbox = NULL;
}

static void
attach_per_account_boxes()
{
	PidginBuddyList *gtkblist;
	GList *list, *iter;
	GtkWidget *sw2, *vpane, *vbox;
	gboolean headline_showing;

	gtkblist = pidgin_blist_get_default_gtk_blist();

	if (!gtkblist || !gtkblist->window || gtkblist_statusboxbox)
		return;

	headline_showing = GTK_WIDGET_VISIBLE(gtkblist->headline_hbox) && GTK_WIDGET_DRAWABLE(gtkblist->headline_hbox);

	gtkblist_statusboxbox = gtk_vbox_new(FALSE, 0);
	gtkblist_statusboxes = NULL;

	list = purple_accounts_get_all_active();
	for (iter = list; iter; iter = iter->next)
	{
		account_enabled_cb(iter->data, NULL);
	}
	g_list_free(list);

	gtk_widget_show_all(gtkblist_statusboxbox);

	list = gtk_container_get_children(GTK_CONTAINER(gtkblist->vbox));
	for (iter = list; iter; iter = iter->next)
	{
		if (!GTK_IS_SEPARATOR(iter->data))
			g_object_ref(iter->data);
		gtk_container_remove(GTK_CONTAINER(gtkblist->vbox), iter->data);
	}

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gtkblist->headline_hbox->parent, FALSE, FALSE, 0);
	g_object_unref(G_OBJECT(gtkblist->headline_hbox->parent));
	gtk_box_pack_start(GTK_BOX(vbox), gtkblist->treeview->parent, TRUE, TRUE, 0);
	g_object_unref(G_OBJECT(gtkblist->treeview->parent));
	gtk_box_pack_start(GTK_BOX(vbox), gtk_hseparator_new(), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gtkblist->scrollbook, FALSE, FALSE, 0);
	g_object_unref(G_OBJECT(gtkblist->scrollbook));
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
	if (purple_prefs_get_bool(PREF_GLOBAL))
		gtk_widget_hide(gtkblist->statusbox);
	else
		gtk_widget_show(gtkblist->statusbox);
	
	if (!headline_showing)
		gtk_widget_hide(gtkblist->headline_hbox);

	g_object_set(gtkblist->statusbox, "iconsel", !purple_prefs_get_bool(PREF_ICONS_HIDE), NULL);

	pidgin_status_selectors_show(purple_prefs_get_int(PREF_SHOW));

	gtk_paned_set_position(GTK_PANED(vpane), purple_prefs_get_int(PREF_PANE));
	g_signal_connect(G_OBJECT(vpane), "notify::position",
							G_CALLBACK(pane_position_cb), NULL);
}

static void
pidgin_status_selectors_show(PurpleStatusBoxVisibility action)
{
	GtkRequisition req;
	int height;
	GList *list;
	PidginBuddyList *gtkblist;

	gtkblist = pidgin_blist_get_default_gtk_blist();

	purple_prefs_set_int(PREF_SHOW, action);

	if (!gtkblist || !gtkblist_statusboxbox)
		return;

	/* XXX: CHANGE THIS WHEN PIDGIN FINISHES ITS PREFS MIGRATION!!!! */
	height = purple_prefs_get_int("/pidgin/blist/height");
	
	if (!purple_prefs_get_bool(PREF_GLOBAL))
	{
		gtk_widget_size_request(gtkblist->statusbox, &req);
		height -= req.height;	/* Height of the global statusbox */
	}

	list = gtkblist_statusboxes;
	for (; list; list = list->next)
	{
		GtkWidget *box = list->data;

		if (action == PURPLE_STATUSBOX_ALL)	/* Show all */
		{
			gtk_widget_show_all(box);
			gtk_widget_size_request(box, &req);
			height -= req.height;
		}
		else if (action == PURPLE_STATUSBOX_NONE)	/* Show none */
		{
			gtk_widget_hide_all(box);
		}
		else if (action == PURPLE_STATUSBOX_OUT_SYNC)	/* Show non-synced ones */
		{
			PurpleAccount *account = PIDGIN_STATUS_BOX(box)->account;
			PurpleStatus *status;
			PurpleStatusPrimitive account_prim, global_prim;
			PurpleSavedStatus *saved;
			PurpleSavedStatusSub *sub;
			gboolean show = TRUE;
			const char *global_message;

			/* This, unfortunately, is necessary, until (if at all) #1440568 gets in */
			if (!purple_account_is_connected(account))
				status = purple_account_get_status(account, "offline");
			else
				status = purple_account_get_active_status(account);

			account_prim = purple_status_type_get_primitive(purple_status_get_type(status));

			saved = purple_savedstatus_get_current();
			sub = purple_savedstatus_get_substatus(saved, account);
			if (sub)
			{
				global_prim = purple_status_type_get_primitive(purple_savedstatus_substatus_get_type(sub));
				global_message = purple_savedstatus_substatus_get_message(sub);
			}
			else
			{
				global_prim = purple_savedstatus_get_type(saved);
				global_message = purple_savedstatus_get_message(saved);
			}

			if (global_prim == account_prim)
			{
				if (purple_status_type_get_attr(purple_status_get_type(status), "message"))
				{
					const char *message = NULL;

					message = purple_status_get_attr_string(status, "message");

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

	if (GTK_WIDGET_DRAWABLE(gtkblist->scrollbook) && GTK_WIDGET_VISIBLE(gtkblist->scrollbook)) {
		gtk_widget_size_request(gtkblist->scrollbook, &req);
		height -= req.height;	/* Height of the scrollbook */
		height -= 3;  /* the separator */
	}

	if (GTK_WIDGET_VISIBLE(gtkblist->menutray->parent)) {
		gtk_widget_size_request(gtkblist->menutray->parent, &req);
		height -= req.height;  /* Height of the menu */
	}

	height -= 9;	/* Height of the handle of the vpane */

	gtk_paned_set_position(GTK_PANED(gtkblist_statusboxbox->parent->parent->parent), height);
}

static void
show_boxes_all(PurplePluginAction *action)
{
	pidgin_status_selectors_show(PURPLE_STATUSBOX_ALL);
}

static void
show_boxes_none(PurplePluginAction *action)
{
	pidgin_status_selectors_show(PURPLE_STATUSBOX_NONE);
}

static void
show_boxes_out_of_sync(PurplePluginAction *action)
{
	pidgin_status_selectors_show(PURPLE_STATUSBOX_OUT_SYNC);
}

static void
toggle_icons(PurplePluginAction *action)
{
	gboolean v = purple_prefs_get_bool(PREF_ICONS_HIDE);
	purple_prefs_set_bool(PREF_ICONS_HIDE, !v);
}

static void
toggle_global(PurplePluginAction *action)
{
	gboolean v = purple_prefs_get_bool(PREF_GLOBAL);
	purple_prefs_set_bool(PREF_GLOBAL, !v);
}

static GList *
actions(PurplePlugin *plugin, gpointer context)
{
	GList *l = NULL;
	PurplePluginAction *act = NULL;

	act = purple_plugin_action_new(_("All"), show_boxes_all);
	l = g_list_append(l, act);

	act = purple_plugin_action_new(_("None"), show_boxes_none);
	l = g_list_append(l, act);

	act = purple_plugin_action_new(_("Out of sync ones"), show_boxes_out_of_sync);
	l = g_list_append(l, act);

	l = g_list_append(l, NULL);
	
	act = purple_plugin_action_new(_("Toggle icon selectors"), toggle_icons);
	l = g_list_append(l, act);

	act = purple_plugin_action_new(_("Toggle global selector"), toggle_global);
	l = g_list_append(l, act);

	return l;
}

static void
toggle_iconsel_cb(const char *name, PurplePrefType type, gconstpointer val, gpointer null)
{
	GList *iter = gtkblist_statusboxes;
	gboolean value = !GPOINTER_TO_INT(val);
	PidginBuddyList *gtkblist;

	for (; iter; iter = iter->next)
	{
		g_object_set(iter->data, "iconsel", value, NULL);
	}

	gtkblist = pidgin_blist_get_default_gtk_blist();
	if (gtkblist)
		g_object_set(gtkblist->statusbox, "iconsel", value, NULL);
}

static void
hide_global_callback(const char *name, PurplePrefType type, gconstpointer val, gpointer null)
{
	PidginBuddyList *gtkblist = pidgin_blist_get_default_gtk_blist();
	gboolean hide = GPOINTER_TO_UINT(val);

	if (!gtkblist)
		return;

	if (hide)
		gtk_widget_hide(gtkblist->statusbox);
	else
		gtk_widget_show(gtkblist->statusbox);
}

static void
account_status_changed_cb(PurpleAccount *account, PurpleStatus *os, PurpleStatus *ns, gpointer null)
{
	update_out_of_sync();
}

static void
global_status_changed_cb(const char *name, PurplePrefType type, gconstpointer val, gpointer null)
{
	update_out_of_sync();
}

static void
signed_on_off_cb(PurpleConnection *gc, gpointer null)
{
	update_out_of_sync();
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	attach_per_account_boxes();
	
	purple_signal_connect(pidgin_blist_get_handle(), "gtkblist-created",
				plugin, PURPLE_CALLBACK(attach_per_account_boxes), NULL);
	purple_signal_connect(purple_accounts_get_handle(), "account-added", plugin,
				PURPLE_CALLBACK(account_enabled_cb), NULL);
	purple_signal_connect(purple_accounts_get_handle(), "account-enabled", plugin,
				PURPLE_CALLBACK(account_enabled_cb), NULL);
	purple_signal_connect(purple_accounts_get_handle(), "account-removed", plugin,
				PURPLE_CALLBACK(account_disabled_cb), NULL);
	purple_signal_connect(purple_accounts_get_handle(), "account-disabled", plugin,
				PURPLE_CALLBACK(account_disabled_cb), NULL);
	purple_signal_connect(purple_accounts_get_handle(), "account-status-changed", plugin,
				PURPLE_CALLBACK(account_status_changed_cb), NULL);
	purple_signal_connect(purple_connections_get_handle(), "signed-on", plugin,
				PURPLE_CALLBACK(signed_on_off_cb), NULL);
	purple_signal_connect(purple_connections_get_handle(), "signed-off", plugin,
				PURPLE_CALLBACK(signed_on_off_cb), NULL);
		
	purple_prefs_connect_callback(plugin, PREF_GLOBAL, hide_global_callback, NULL);
	purple_prefs_connect_callback(plugin, PREF_ICONS_HIDE, toggle_iconsel_cb, NULL);
	purple_prefs_connect_callback(plugin, "/core/savedstatus/current", global_status_changed_cb, NULL);
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	PidginBuddyList *gtkblist;
	
	pidgin_status_selectors_show(PURPLE_STATUSBOX_ALL);
	detach_per_account_boxes();

	gtkblist = pidgin_blist_get_default_gtk_blist();
	if (gtkblist)
		gtk_widget_show(gtkblist->statusbox);
	purple_prefs_disconnect_by_handle(plugin);
	
	return TRUE;
}

static PurplePluginPrefFrame *
get_plugin_pref_frame(PurplePlugin *plugin)
{
	PurplePluginPrefFrame *frame;
	PurplePluginPref *pref;

	frame = purple_plugin_pref_frame_new();

	pref = purple_plugin_pref_new_with_name_and_label(PREF_GLOBAL, _("Hide global status selector"));
	purple_plugin_pref_frame_add(frame, pref);

	pref = purple_plugin_pref_new_with_name_and_label(PREF_ICONS_HIDE, _("Hide icon-selectors"));
	purple_plugin_pref_frame_add(frame, pref);

	return frame;
}

static PurplePluginUiInfo prefs_info = {
	get_plugin_pref_frame,
	0,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,			/* Magic				*/
	PURPLE_MAJOR_VERSION,			/* Purple Major Version	*/
	PURPLE_MINOR_VERSION,			/* Purple Minor Version	*/
	PURPLE_PLUGIN_STANDARD,			/* plugin type			*/
	PIDGIN_PLUGIN_TYPE,				/* ui requirement		*/
	0,								/* flags				*/
	NULL,							/* dependencies			*/
	PURPLE_PRIORITY_DEFAULT,		/* priority				*/

	PLUGIN_ID,						/* plugin id			*/
	NULL,							/* name					*/
	PP_VERSION,						/* version				*/
	NULL,							/* summary				*/
	NULL,							/* description			*/
	PLUGIN_AUTHOR,					/* author				*/
	PP_WEBSITE,						/* website				*/

	plugin_load,					/* load					*/
	plugin_unload,					/* unload				*/
	NULL,							/* destroy				*/

	NULL,							/* ui_info				*/
	NULL,							/* extra_info			*/
	&prefs_info,					/* prefs_info			*/
	actions,						/* actions				*/
	NULL,							/* reserved 1			*/
	NULL,							/* reserved 2			*/
	NULL,							/* reserved 3			*/
	NULL							/* reserved 4			*/
};

static void
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _(PLUGIN_NAME);
	info.summary = _(PLUGIN_SUMMARY);
	info.description = _(PLUGIN_DESCRIPTION);

	purple_prefs_add_none(PREF_PREFIX);
	purple_prefs_add_int(PREF_PANE, 300);
	purple_prefs_add_bool(PREF_GLOBAL, FALSE);
	purple_prefs_add_int(PREF_SHOW, PURPLE_STATUSBOX_ALL);
	purple_prefs_add_bool(PREF_ICONS_HIDE, FALSE);
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info)
#endif

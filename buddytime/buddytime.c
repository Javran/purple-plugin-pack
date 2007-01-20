/*
 * Plugin Name - Summary
 * Copyright (C) 2004
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
# include "../gpp_config.h"
#endif

#define GAIM_PLUGINS

#define PLUGIN_ID			"amc_grim-buddytime-gtk"
#define PLUGIN_NAME			"Buddy Time"
#define PLUGIN_STATIC_NAME	"buddytime"
#define PLUGIN_SUMMARY		"summary"
#define PLUGIN_DESCRIPTION	"description"
#define PLUGIN_AUTHOR		"Gary Kramlich <grim@reaperworld.com>"

#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>

#include <gaim/version.h>
#include <gaim/blist.h>
#include <gaim/gtkutils.h>
#include <gaim/gtkplugin.h>
#include <gaim/request.h>

#include "../common/i18n.h"

/******************************************************************************
 * Defines
 *****************************************************************************/
#define BT_NODE_SETTING	"bt-timezone"

/******************************************************************************
 * Structures
 *****************************************************************************/
typedef struct {
	GaimBlistNode *node;
	GaimRequestField *timezone;
	gpointer handle;
} BTDialog;

typedef struct {
	GtkWidget *ebox;
	GtkWidget *label;
	GaimConversation *conv;
	guint timeout;
} BTWidget;

/******************************************************************************
 * Globals
 *****************************************************************************/
static GList *dialogs = NULL;
static GList *widgets = NULL;

/******************************************************************************
 * Main Stuff
 *****************************************************************************/
static BTWidget *
bt_widget_new(GaimConversation *conv) {
	BTWidget *ret;

	g_return_val_if_fail(conv, NULL);

	ret = g_new0(BTWidget, 1);
	ret->conv = conv;

	ret->ebox = gtk_event_box_new();

	ret->label = gtk_label_new("label");
	gtk_container_add(GTK_CONTAINER(ret->ebox), ret->label);
}

/******************************************************************************
 * Blist Stuff
 *****************************************************************************/
static void
bt_dialog_ok_cb(gpointer data, GaimRequestFields *fields) {
	BTDialog *dialog = (BTDialog *)data;

	dialogs = g_list_remove(dialogs, dialog);
	g_free(dialog);
}

static void
bt_dialog_cancel_cb(gpointer data, GaimRequestFields *fields) {
	BTDialog *dialog = (BTDialog *)data;

	dialogs = g_list_remove(dialogs, dialog);
	g_free(dialog);
}

static void
bt_show_dialog(GaimBlistNode *node) {
	BTDialog *dialog;
	GaimRequestFields *fields;
	GaimRequestFieldGroup *group;
	gint current = 0;

	dialog = g_new0(BTDialog, 1);

	if(!dialog)
		return;

	dialog->node = node;

	current = gaim_blist_node_get_int(node, BT_NODE_SETTING);

	/* build the request fields */
	fields = gaim_request_fields_new();
	group = gaim_request_field_group_new(NULL);
	gaim_request_fields_add_group(fields, group);

	dialog->timezone = gaim_request_field_choice_new("timezone",
													 _("_Timezone"), 1);
	gaim_request_field_group_add_field(group, dialog->timezone);

	gaim_request_field_choice_add(dialog->timezone, _("Clear setting"));

	gaim_request_field_choice_add(dialog->timezone, _("GMT-12"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT-11"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT-10"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT-9"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT-8"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT-7"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT-6"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT-5"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT-4"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT-3"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT-2"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT-1"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT+1"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT+2"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT+3"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT+4"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT+5"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT+6"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT+7"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT+8"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT+9"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT+10"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT+11"));
	gaim_request_field_choice_add(dialog->timezone, _("GMT+12"));

//	gaim_request_field_choice_set_default_value(dialog->timezone, current);
//	gaim_request_field_coice_set_value(dialog->timezone, current);

	dialog->handle =
		gaim_request_fields(NULL, _("Select timezone"),
							NULL, "foo", fields,
							_("OK"), GAIM_CALLBACK(bt_dialog_ok_cb),
							_("Cancel"), GAIM_CALLBACK(bt_dialog_cancel_cb),
							dialog);
	
	dialogs = g_list_append(dialogs, dialog);
}

static void
bt_edit_timezone_cb(GaimBlistNode *node, gpointer data) {
	bt_show_dialog(node);
}

static void
bt_blist_drawing_menu_cb(GaimBlistNode *node, GList **menu) {
	GaimMenuAction *action;

	/* ignore chats and groups */
	if(GAIM_BLIST_NODE_IS_CHAT(node) || GAIM_BLIST_NODE_IS_GROUP(node))
		return;
	
	action = gaim_menu_action_new(_("Timezone"),
								  GAIM_CALLBACK(bt_edit_timezone_cb),
								  NULL,
								  NULL);
	(*menu) = g_list_append(*menu, action);
}

/******************************************************************************
 * Conversation stuff
 *****************************************************************************/


/******************************************************************************
 * Plugin Stuff
 *****************************************************************************/
static gboolean
plugin_load(GaimPlugin *plugin) {
	gaim_signal_connect(gaim_blist_get_handle(),
						"blist-node-extended-menu",
						plugin,
						GAIM_CALLBACK(bt_blist_drawing_menu_cb),
						NULL);

	return TRUE;
}

static gboolean
plugin_unload(GaimPlugin *plugin) {
	return TRUE;
}

static GaimPluginInfo info = {
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
	NULL,						/* prefs_info			*/
	NULL						/* actions				*/
};

static void
init_plugin(GaimPlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _(PLUGIN_NAME);
	info.summary = _(PLUGIN_SUMMARY);
	info.description = _(PLUGIN_DESCRIPTION);
}

GAIM_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info)

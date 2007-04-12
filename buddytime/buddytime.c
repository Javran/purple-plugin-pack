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

#define PURPLE_PLUGINS

#define PLUGIN_ID			"amc_grim-buddytime-gtk"
#define PLUGIN_NAME			"Buddy Time"
#define PLUGIN_STATIC_NAME	"buddytime"
#define PLUGIN_SUMMARY		"summary"
#define PLUGIN_DESCRIPTION	"description"
#define PLUGIN_AUTHOR		"Gary Kramlich <grim@reaperworld.com>"

#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>

#include <version.h>
#include <blist.h>
#include <gtkutils.h>
#include <gtkplugin.h>
#include <request.h>

#include "../common/i18n.h"

/******************************************************************************
 * Defines
 *****************************************************************************/
#define BT_NODE_SETTING	"bt-timezone"

/******************************************************************************
 * Structures
 *****************************************************************************/
typedef struct {
	PurpleBlistNode *node;
	PurpleRequestField *timezone;
	gpointer handle;
} BTDialog;

typedef struct {
	GtkWidget *ebox;
	GtkWidget *label;
	PurpleConversation *conv;
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
bt_widget_new(PurpleConversation *conv) {
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
bt_dialog_ok_cb(gpointer data, PurpleRequestFields *fields) {
	BTDialog *dialog = (BTDialog *)data;

	dialogs = g_list_remove(dialogs, dialog);
	g_free(dialog);
}

static void
bt_dialog_cancel_cb(gpointer data, PurpleRequestFields *fields) {
	BTDialog *dialog = (BTDialog *)data;

	dialogs = g_list_remove(dialogs, dialog);
	g_free(dialog);
}

static void
bt_show_dialog(PurpleBlistNode *node) {
	BTDialog *dialog;
	PurpleRequestFields *fields;
	PurpleRequestFieldGroup *group;
	gint current = 0;

	dialog = g_new0(BTDialog, 1);

	if(!dialog)
		return;

	dialog->node = node;

	current = purple_blist_node_get_int(node, BT_NODE_SETTING);

	/* build the request fields */
	fields = purple_request_fields_new();
	group = purple_request_field_group_new(NULL);
	purple_request_fields_add_group(fields, group);

	dialog->timezone = purple_request_field_choice_new("timezone",
													 _("_Timezone"), 1);
	purple_request_field_group_add_field(group, dialog->timezone);

	purple_request_field_choice_add(dialog->timezone, _("Clear setting"));

	purple_request_field_choice_add(dialog->timezone, _("GMT-12"));
	purple_request_field_choice_add(dialog->timezone, _("GMT-11"));
	purple_request_field_choice_add(dialog->timezone, _("GMT-10"));
	purple_request_field_choice_add(dialog->timezone, _("GMT-9"));
	purple_request_field_choice_add(dialog->timezone, _("GMT-8"));
	purple_request_field_choice_add(dialog->timezone, _("GMT-7"));
	purple_request_field_choice_add(dialog->timezone, _("GMT-6"));
	purple_request_field_choice_add(dialog->timezone, _("GMT-5"));
	purple_request_field_choice_add(dialog->timezone, _("GMT-4"));
	purple_request_field_choice_add(dialog->timezone, _("GMT-3"));
	purple_request_field_choice_add(dialog->timezone, _("GMT-2"));
	purple_request_field_choice_add(dialog->timezone, _("GMT-1"));
	purple_request_field_choice_add(dialog->timezone, _("GMT"));
	purple_request_field_choice_add(dialog->timezone, _("GMT+1"));
	purple_request_field_choice_add(dialog->timezone, _("GMT+2"));
	purple_request_field_choice_add(dialog->timezone, _("GMT+3"));
	purple_request_field_choice_add(dialog->timezone, _("GMT+4"));
	purple_request_field_choice_add(dialog->timezone, _("GMT+5"));
	purple_request_field_choice_add(dialog->timezone, _("GMT+6"));
	purple_request_field_choice_add(dialog->timezone, _("GMT+7"));
	purple_request_field_choice_add(dialog->timezone, _("GMT+8"));
	purple_request_field_choice_add(dialog->timezone, _("GMT+9"));
	purple_request_field_choice_add(dialog->timezone, _("GMT+10"));
	purple_request_field_choice_add(dialog->timezone, _("GMT+11"));
	purple_request_field_choice_add(dialog->timezone, _("GMT+12"));

//	purple_request_field_choice_set_default_value(dialog->timezone, current);
//	purple_request_field_coice_set_value(dialog->timezone, current);

	dialog->handle =
		purple_request_fields(NULL, _("Select timezone"),
							NULL, "foo", fields,
							_("OK"), PURPLE_CALLBACK(bt_dialog_ok_cb),
							_("Cancel"), PURPLE_CALLBACK(bt_dialog_cancel_cb),
							dialog);
	
	dialogs = g_list_append(dialogs, dialog);
}

static void
bt_edit_timezone_cb(PurpleBlistNode *node, gpointer data) {
	bt_show_dialog(node);
}

static void
bt_blist_drawing_menu_cb(PurpleBlistNode *node, GList **menu) {
	PurpleMenuAction *action;

	/* ignore chats and groups */
	if(PURPLE_BLIST_NODE_IS_CHAT(node) || PURPLE_BLIST_NODE_IS_GROUP(node))
		return;
	
	action = purple_menu_action_new(_("Timezone"),
								  PURPLE_CALLBACK(bt_edit_timezone_cb),
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
plugin_load(PurplePlugin *plugin) {
	purple_signal_connect(purple_blist_get_handle(),
						"blist-node-extended-menu",
						plugin,
						PURPLE_CALLBACK(bt_blist_drawing_menu_cb),
						NULL);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin) {
	return TRUE;
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,			/* Magic				*/
	PURPLE_MAJOR_VERSION,			/* Purple Major Version	*/
	PURPLE_MINOR_VERSION,			/* Purple Minor Version	*/
	PURPLE_PLUGIN_STANDARD,		/* plugin type			*/
	PIDGIN_PLUGIN_TYPE,		/* ui requirement		*/
	0,							/* flags				*/
	NULL,						/* dependencies			*/
	PURPLE_PRIORITY_DEFAULT,		/* priority				*/

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
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _(PLUGIN_NAME);
	info.summary = _(PLUGIN_SUMMARY);
	info.description = _(PLUGIN_DESCRIPTION);
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info)

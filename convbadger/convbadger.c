/*
 * ConvBadger - Adds the protocol icon to the menu tray of a conversation
 * Copyright (C) 2007 Gary Kramlich <grim@reaperworld.com>
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

/* If you can't figure out what this line is for, DON'T TOUCH IT. */
#include "../common/pp_internal.h"

#include <gtk/gtk.h>

#include <conversation.h>
#include <signals.h>

#include <gtkmenutray.h>
#include <gtkplugin.h>
#include <gtkutils.h>

/******************************************************************************
 * Structs
 *****************************************************************************/
typedef struct {
	PidginWindow *win;
	PurpleConversation *conv;
	GtkWidget *icon;
} ConvBadgerData;

/******************************************************************************
 * Globals
 *****************************************************************************/
static GHashTable *data = NULL;

/******************************************************************************
 * helpers
 *****************************************************************************/
static void
conv_badger_data_free(ConvBadgerData *cbd) {
	cbd->win = NULL;
	cbd->conv = NULL;

	if(cbd->icon && GTK_IS_IMAGE(cbd->icon))
		gtk_widget_destroy(cbd->icon);

	g_free(cbd);

	cbd = NULL;
}

static void
conv_badger_data_free_helper(gpointer k, gpointer v, gpointer d) {
	ConvBadgerData *cbd = (ConvBadgerData *)v;

	conv_badger_data_free(cbd);
}

static void
conv_badger_update(PidginWindow *win, PurpleConversation *conv) {
	ConvBadgerData *cbd = NULL;
	GdkPixbuf *pixbuf = NULL;
	PurpleAccount *account = NULL;

	g_return_if_fail(win);
	g_return_if_fail(conv);

	cbd = g_hash_table_lookup(data, win);

	if(cbd == NULL) {
		cbd = g_new0(ConvBadgerData, 1);

		cbd->win = win;

		cbd->icon = gtk_image_new();
		pidgin_menu_tray_append(PIDGIN_MENU_TRAY(win->menu.tray), cbd->icon,
								NULL);
		gtk_widget_show(cbd->icon);

		g_signal_connect_swapped(G_OBJECT(cbd->icon), "destroy",
				G_CALLBACK(g_nullify_pointer), &cbd->icon);
	}

	cbd->conv = conv;

	account = purple_conversation_get_account(conv);
	pixbuf = pidgin_create_prpl_icon(account, PIDGIN_PRPL_ICON_SMALL);
	gtk_image_set_from_pixbuf(GTK_IMAGE(cbd->icon), pixbuf);
	g_object_unref(G_OBJECT(pixbuf));

	g_hash_table_insert(data, win, cbd);
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
convbadger_conv_created_cb(PurpleConversation *conv, gpointer data) {
	PidginConversation *pconv = PIDGIN_CONVERSATION(conv);
	PidginWindow *win = pidgin_conv_get_window(pconv);

	conv_badger_update(win, conv);
}

static void
convbadger_conv_destroyed_cb(PurpleConversation *conv, gpointer data) {
}

static void
convbadger_conv_switched_cb(PurpleConversation *conv, gpointer data) {
	PidginConversation *pconv = PIDGIN_CONVERSATION(conv);
	PidginWindow *win = pidgin_conv_get_window(pconv);

	conv_badger_update(win, conv);
}

/******************************************************************************
 * Plugin Stuff
 *****************************************************************************/
static gboolean
plugin_load(PurplePlugin *plugin) {
	void *conv_handle = purple_conversations_get_handle();

	data = g_hash_table_new_full(g_direct_hash, g_direct_equal,
								 NULL, NULL);

	purple_signal_connect(conv_handle, "conversation-created", plugin,
						  PURPLE_CALLBACK(convbadger_conv_created_cb), NULL);
	purple_signal_connect(conv_handle, "deleting-conversation", plugin,
						  PURPLE_CALLBACK(convbadger_conv_destroyed_cb), NULL);

	purple_signal_connect(pidgin_conversations_get_handle(),
						  "conversation-switched", plugin,
						  PURPLE_CALLBACK(convbadger_conv_switched_cb), NULL);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin) {
	g_hash_table_foreach(data, conv_badger_data_free_helper, NULL);

	g_hash_table_destroy(data);

	data = NULL;

	return TRUE;
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	PIDGIN_PLUGIN_TYPE,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,

	"gtk-plugin_pack-convbadger",
	NULL,
	PP_VERSION,
	NULL,
	NULL,
	"Gary Kramlich <grim@reaperworld.com>",	
	PP_WEBSITE,	

	plugin_load,
	plugin_unload,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL
};


static void
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Conversation Badger");
	info.summary = _("Badges conversations with the protocol icon.");
	info.description = _("Badges conversations with the protocol icon.");
}

PURPLE_INIT_PLUGIN(convbadger, init_plugin, info)

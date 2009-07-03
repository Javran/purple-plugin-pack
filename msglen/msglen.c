/*
 * msglen - Adds the current message's length to the menutray of a conversation
 * Copyright (C) 2008 Gary Kramlich <grim@reaperworld.com>
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

#include <gtkconv.h>
#include <gtkmenutray.h>
#include <gtkplugin.h>
#include <gtkutils.h>

/******************************************************************************
 * Structs
 *****************************************************************************/
typedef struct {
	PurpleConversation *conv;
	PidginWindow *win;
	GtkWidget *label;
	gulong sig_id;
} MsgLenData;

/******************************************************************************
 * Globals
 *****************************************************************************/
static GHashTable *data = NULL;

/******************************************************************************
 * helpers
 *****************************************************************************/
static void
msg_len_data_free(MsgLenData *mld) {
	mld->win = NULL;

	if(mld->label && GTK_IS_LABEL(mld->label))
		gtk_widget_destroy(mld->label);

	g_free(mld);

	mld = NULL;
}

static void
msg_len_data_free_helper(gpointer k, gpointer v, gpointer d) {
	MsgLenData *mld = (MsgLenData *)v;

	msg_len_data_free(mld);
}

static void msg_len_update(PidginWindow *win, PurpleConversation *conv);

static gboolean
msg_len_key_released_cb(GtkWidget *w,  GdkEventKey *e, gpointer d) {
	MsgLenData *mld = (MsgLenData *)d;

	if(d)
		msg_len_update(mld->win, mld->conv);

	return FALSE;
}

static void
msg_len_add_signal(PidginConversation *pconv, MsgLenData *mld) {
	g_signal_connect(G_OBJECT(pconv->entry),
					 "key-release-event",
					 G_CALLBACK(msg_len_key_released_cb), mld);
}

static MsgLenData *
msg_len_find_data(PidginWindow *win) {
	MsgLenData *mld = NULL;

	mld = g_hash_table_lookup(data, win);

	if(mld == NULL) {
		mld = g_new0(MsgLenData, 1);

		mld->win = win;

		mld->label = gtk_label_new("");
		pidgin_menu_tray_append(PIDGIN_MENU_TRAY(win->menu.tray), mld->label,
								NULL);
		gtk_widget_show(mld->label);

		g_signal_connect_swapped(G_OBJECT(mld->label), "destroy",
								 G_CALLBACK(g_nullify_pointer), &mld->label);
	}

	return mld;
}

static void
msg_len_update(PidginWindow *win, PurpleConversation *conv) {
	PidginConversation *gtkconv = NULL;
	MsgLenData *mld = NULL;
	gchar *text = NULL;
	gint count = 0;

	g_return_if_fail(win);
	g_return_if_fail(conv);

	gtkconv = PIDGIN_CONVERSATION(conv);

	mld = msg_len_find_data(win);

	mld->conv = conv;

	count = gtk_text_buffer_get_char_count(gtkconv->entry_buffer);

	text = g_strdup_printf("%d", count);
	gtk_label_set_text(GTK_LABEL(mld->label), text);
	g_free(text);

	g_hash_table_insert(data, win, mld);
}

/******************************************************************************
 * Callbacks
 *****************************************************************************/
static void
msg_len_conv_created_cb(PurpleConversation *conv, gpointer d) {
	PidginConversation *pconv = PIDGIN_CONVERSATION(conv);
	PidginWindow *win = pidgin_conv_get_window(pconv);
	MsgLenData *mld = NULL;

	mld = msg_len_find_data(win);

	msg_len_add_signal(pconv, mld);

	msg_len_update(win, conv);
}

static void
msg_len_conv_destroyed_cb(PurpleConversation *conv, gpointer d) {
	g_hash_table_remove(data, conv);
}

static void
msg_len_conv_switched_cb(PurpleConversation *conv, gpointer d) {
	PidginConversation *pconv = PIDGIN_CONVERSATION(conv);
	PidginWindow *win = pidgin_conv_get_window(pconv);

	msg_len_update(win, conv);
}

/******************************************************************************
 * Plugin Stuff
 *****************************************************************************/
static gboolean
plugin_load(PurplePlugin *plugin) {
	GList *convs = NULL;
	void *conv_handle = purple_conversations_get_handle();

	data = g_hash_table_new_full(g_direct_hash, g_direct_equal,
								 NULL, NULL);

	purple_signal_connect(conv_handle, "conversation-created", plugin,
						  PURPLE_CALLBACK(msg_len_conv_created_cb), NULL);
	purple_signal_connect(conv_handle, "deleting-conversation", plugin,
						  PURPLE_CALLBACK(msg_len_conv_destroyed_cb), NULL);

	purple_signal_connect(pidgin_conversations_get_handle(),
						  "conversation-switched", plugin,
						  PURPLE_CALLBACK(msg_len_conv_switched_cb), NULL);

	for(convs = purple_get_conversations(); convs; convs = convs->next) {
		PurpleConversation *conv = (PurpleConversation *)convs->data;
		PidginConversation *pconv = PIDGIN_CONVERSATION(conv);
		MsgLenData *mld = msg_len_find_data(pconv->win);

		msg_len_add_signal(pconv, mld);

		msg_len_update(pconv->win, conv);
	}

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin) {
	g_hash_table_foreach(data, msg_len_data_free_helper, NULL);

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

	"gtk-plugin_pack-msglen",
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

	info.name = _("Message Length");
	info.summary = _("Shows the length of your current message in the menu "
					 "tray");
	info.description = info.summary;
}

PURPLE_INIT_PLUGIN(msg_len, init_plugin, info)


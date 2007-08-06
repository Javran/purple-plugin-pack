/*
 * Hide Conversations - You can hide conversations without having to close them.
 * Copyright (C) 2007
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

#include "../common/pp_internal.h"

#define PLUGIN_ID			"gtk-plugin_pack-hideconv"
#define PLUGIN_NAME			"Hide Conversation"
#define PLUGIN_STATIC_NAME	"Hide Conversation"
#define PLUGIN_SUMMARY		"Hide conversations without closing them."
#define PLUGIN_DESCRIPTION	"Hide conversations without closing them."
#define PLUGIN_AUTHOR		"Sadrul H Chowdhury <sadrul@users.sourceforge.net>"

/* System headers */
#include <gdk/gdk.h>
#include <gtk/gtk.h>

/* Purple headers */
#include <gtkconv.h>
#include <gtkconvwin.h>
#include <gtkplugin.h>
#include <gtkutils.h>

#define MENUSET "hideconv::menuset"

static PidginWindow *window = NULL;
static void (*orig_conv_present)(PurpleConversation *conv);

static void conv_created(PurpleConversation *conv, gpointer null);

static void
create_hidden_convwin()
{
	/* This is a 'wee bit' hacky. Create two conv windows, remove the second
	 * one from the list, and then destroy the first one. This is because we
	 * want to hide this entire conversation window from pidgin itself.
	 */
	GList *null;
	PidginWindow *tmp = pidgin_conv_window_new();
	window = pidgin_conv_window_new();
	null = pidgin_conv_windows_get_list();
	null = g_list_remove(null, window);
	pidgin_conv_window_hide(window);
	pidgin_conv_window_destroy(tmp);
}

static void
gtkconv_redisplaying(PidginConversation *gtkconv)
{
	conv_created(gtkconv->active_conv, NULL);
	g_signal_handlers_disconnect_by_func(G_OBJECT(gtkconv->imhtml),
			G_CALLBACK(gtkconv_redisplaying), gtkconv);
}

static void
hide_gtkconv(PidginConversation *gtkconv)
{
	pidgin_conv_window_add_gtkconv(window, gtkconv);
	g_signal_connect_swapped(G_OBJECT(gtkconv->imhtml), "visibility_notify_event",
			G_CALLBACK(gtkconv_redisplaying), gtkconv);
}

static void
hide_conv_cb(GtkWidget *wid, PidginWindow *win)
{
	PidginConversation *gtkconv = pidgin_conv_window_get_active_gtkconv(win);
	pidgin_conv_window_remove_gtkconv(win, gtkconv);
	hide_gtkconv(gtkconv);
}

static void
show_convs_cb(PurplePluginAction *dontcare)
{
	GList *list = g_list_copy(pidgin_conv_window_get_gtkconvs(window)), *iter;

	for (iter = list; iter; iter = iter->next) {
		PidginConversation *gtkconv = iter->data;
		pidgin_conv_window_remove_gtkconv(window, gtkconv);
		pidgin_conv_placement_place(gtkconv);
		purple_conversation_present(gtkconv->active_conv);
		conv_created(gtkconv->active_conv, NULL);
	}
	g_list_free(list);

	create_hidden_convwin();
}

static void
attach_menu_to_window(PidginWindow *win)
{
	GtkWidget *widget, *item;
	if (g_object_get_data(G_OBJECT(win->window), MENUSET))
		return;
	g_object_set_data(G_OBJECT(win->window), MENUSET, GINT_TO_POINTER(TRUE));

	widget = gtk_item_factory_get_widget(win->menu.item_factory, N_("/Options"));

	/* We cannot use pidgin_separator, unfortunately. */
	item = gtk_separator_menu_item_new();
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(widget), item);
	g_object_set_data(G_OBJECT(item), MENUSET, GINT_TO_POINTER(TRUE));

	item = gtk_menu_item_new_with_mnemonic(_("_Hide Conversation"));
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(hide_conv_cb), win);
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(widget), item);
	g_object_set_data(G_OBJECT(item), MENUSET, GINT_TO_POINTER(TRUE));

	item = gtk_menu_item_new_with_mnemonic(_("Show Hidden Conversations"));
	g_signal_connect_swapped(G_OBJECT(item), "activate", G_CALLBACK(show_convs_cb), NULL);
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(widget), item);
	g_object_set_data(G_OBJECT(item), MENUSET, GINT_TO_POINTER(TRUE));
}

static void
detach_menu_from_window(PidginWindow *win)
{
	GtkWidget *widget;
	GList *children;
	
	widget = gtk_item_factory_get_widget(win->menu.item_factory, N_("/Options"));
	children = gtk_container_get_children(GTK_CONTAINER(widget));
	while (children) {
		GtkWidget *item = children->data;
		children = children->next;
		if (g_object_get_data(G_OBJECT(item), MENUSET))
			gtk_widget_destroy(item);
	}
	g_object_set_data(G_OBJECT(win->window), MENUSET, GINT_TO_POINTER(FALSE));
}

static gboolean
conv_created_to(PurpleConversation *conv)
{
	PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);

	if (!gtkconv)
		return TRUE;
	if (!GTK_WIDGET_VISIBLE(gtkconv->win->window))
		return TRUE;

	attach_menu_to_window(gtkconv->win);
	return FALSE;
}

static void
conv_created(PurpleConversation *conv, gpointer null)
{
	g_timeout_add(1000, (GSourceFunc)conv_created_to, conv);
}

static void
twisted_present(PurpleConversation *conv)
{
	PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);
	if (gtkconv && gtkconv->win == window) {
		gboolean last = (g_list_length(window->gtkconvs) == 1);
		pidgin_conv_window_remove_gtkconv(window, gtkconv);
		pidgin_conv_placement_place(gtkconv);
		if (last)
			create_hidden_convwin();
	}
	orig_conv_present(conv);
	conv_created(conv, NULL);
}

static void
hide_all_conv(PurplePluginAction *dontcare)
{
	GList *iter = pidgin_conv_windows_get_list();
	while (iter) {
		GList *it = pidgin_conv_window_get_gtkconvs(iter->data);
		iter = iter->next;
		while (it) {
			PidginConversation *gtkconv = it->data;
			it = it->next;
			pidgin_conv_window_remove_gtkconv(gtkconv->win, gtkconv);
			hide_gtkconv(gtkconv);
		}
	}
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	PurpleConversationUiOps *ops = pidgin_conversations_get_conv_ui_ops();
	orig_conv_present = ops->present;
	ops->present = twisted_present;

	create_hidden_convwin();

	purple_signal_connect(purple_conversations_get_handle(), "conversation-created",
			plugin, PURPLE_CALLBACK(conv_created), NULL);

	g_list_foreach(pidgin_conv_windows_get_list(), (GFunc)attach_menu_to_window, NULL);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	PurpleConversationUiOps *ops = pidgin_conversations_get_conv_ui_ops();
	ops->present = orig_conv_present;

	show_convs_cb(NULL);
	pidgin_conv_window_destroy(window);
	window = NULL;

	g_list_foreach(pidgin_conv_windows_get_list(), (GFunc)detach_menu_from_window, NULL);

	return TRUE;
}

static GList *
actions(PurplePlugin *plugin, gpointer context)
{
	PurplePluginAction *act;
	GList *list = NULL;
	
	act = purple_plugin_action_new(_("Show All Hidden Conversations"), show_convs_cb);
	list = g_list_append(list, act);

	act = purple_plugin_action_new(_("Hide All Conversations"), hide_all_conv);
	list = g_list_append(list, act);

	return list;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,       /* Magic              */
	PURPLE_MAJOR_VERSION,      /* Purple Major Version */
	PURPLE_MINOR_VERSION,      /* Purple Minor Version */
	PURPLE_PLUGIN_STANDARD,    /* plugin type        */
	PIDGIN_PLUGIN_TYPE,        /* ui requirement     */
	0,                         /* flags              */
	NULL,                      /* dependencies       */
	PURPLE_PRIORITY_DEFAULT,   /* priority           */

	PLUGIN_ID,                 /* plugin id          */
	NULL,                      /* name               */
	PP_VERSION,                /* version            */
	NULL,                      /* summary            */
	NULL,                      /* description        */
	PLUGIN_AUTHOR,             /* author             */
	PP_WEBSITE,                /* website            */

	plugin_load,               /* load               */
	plugin_unload,             /* unload             */
	NULL,                      /* destroy            */

	NULL,                      /* ui_info            */
	NULL,                      /* extra_info         */
	NULL,                      /* prefs_info         */
	actions,                  /* actions            */
	NULL,                      /* reserved 1         */
	NULL,                      /* reserved 2         */
	NULL,                      /* reserved 3         */
	NULL                       /* reserved 4         */
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

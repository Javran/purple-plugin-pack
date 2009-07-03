/*
  xmms-remote - Control xmms from Pidgin conversations
  Copyright (C) 2004-2008 Gary Kramlich

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301, USA.
*/

/* If you can't figure out what this line is for, DON'T TOUCH IT. */
#include "../common/pp_internal.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <xmmsctrl.h>

#include <string.h>

#include <cmds.h>
#include <conversation.h>
#include <debug.h>
#include <gtkblist.h>
#include <gtkconv.h>
#include <gtkmenutray.h>
#include <gtkplugin.h>
#include <gtkprefs.h>
#include <gtkutils.h>
#include <prefs.h>

/*******************************************************************************
 * Constants
 ******************************************************************************/
#define GXR_SESSION \
	(purple_prefs_get_int("/plugins/gtk/plugin_pack/xmms-remote/session"))

#define GXR_STOCK_NEXT "gxr-next"
#define GXR_STOCK_PAUSE "gxr-pause"
#define GXR_STOCK_PLAY "gxr-play"
#define GXR_STOCK_PREVIOUS "gxr-previous"
#define GXR_STOCK_STOP "gxr-stop"
#define GXR_STOCK_XMMS "gxr-xmms"

enum {
	GXR_PREF_PLAYLIST = 1,
	GXR_PREF_BLIST,
	GXR_PREF_CONV_WINDOW,
	GXR_PREF_EXTENDED_CTRL,
	GXR_PREF_VOLUME_CTRL
};

/*******************************************************************************
 * Globals
 ******************************************************************************/
static GList *buttons = NULL, *checkboxes = NULL;
static GtkIconFactory *icon_factory;
static GtkWidget *blist_button = NULL;
static PurpleCmdId gxr_cmd;

/*******************************************************************************
 * Callbacks
 ******************************************************************************/
static void
gxr_menu_play_cb(GtkMenuItem *item, gpointer data) {
	xmms_remote_play(GXR_SESSION);
}

static void
gxr_menu_pause_cb(GtkMenuItem *item, gpointer data) {
	xmms_remote_pause(GXR_SESSION);
}

static void
gxr_menu_stop_cb(GtkMenuItem *item, gpointer data) {
	xmms_remote_stop(GXR_SESSION);
}

static void
gxr_menu_next_cb(GtkMenuItem *item, gpointer data) {
	xmms_remote_playlist_next(GXR_SESSION);
}

static void
gxr_menu_prev_cb(GtkMenuItem *item, gpointer data) {
	xmms_remote_playlist_prev(GXR_SESSION);
}

static void
gxr_menu_repeat_cb(GtkMenuItem *item, gpointer data) {
	xmms_remote_toggle_repeat(GXR_SESSION);
}

static void
gxr_menu_shuffle_cb(GtkMenuItem *item, gpointer data) {
	xmms_remote_toggle_shuffle(GXR_SESSION);
}

static void
gxr_menu_playlist_cb(GtkMenuItem *item, gpointer data) {
	gint position = GPOINTER_TO_INT(data);

	xmms_remote_set_playlist_pos(GXR_SESSION, position);

	if(!xmms_remote_is_playing(GXR_SESSION))
		xmms_remote_play(GXR_SESSION);
}

static void
gxr_update_volume(GtkWidget *widget, gpointer null) {
	gint value = (gint)gtk_range_get_value(GTK_RANGE(widget));
	xmms_remote_set_main_volume(GXR_SESSION, value);
}

static void
gxr_button_play_cb(GtkWidget *w, GdkEventButton *e, gpointer d) {
	xmms_remote_play(GXR_SESSION);
}

static void
gxr_button_pause_cb(GtkWidget *w, GdkEventButton *e, gpointer d) {
	xmms_remote_pause(GXR_SESSION);
}

static void
gxr_button_stop_cb(GtkWidget *w, GdkEventButton *e, gpointer d) {
	xmms_remote_stop(GXR_SESSION);
}

static void
gxr_button_next_cb(GtkWidget *w, GdkEventButton *e, gpointer d) {
	xmms_remote_playlist_next(GXR_SESSION);
}

static void
gxr_button_prev_cb(GtkWidget *w, GdkEventButton *e, gpointer d) {
	xmms_remote_playlist_prev(GXR_SESSION);
}

/*******************************************************************************
 * Helpers
 ******************************************************************************/
static gchar *
gxr_format_info() {
	GString *str;
	gchar *ret, *song = NULL;
	const gchar *format;
	gint session, rate = 0, freq = 0, chan = 0;
	gint pos, length, volume, time_total, time_elapsed;
	gint min, sec;

	session = GXR_SESSION;
	
	pos = xmms_remote_get_playlist_pos(session);
	time_total= xmms_remote_get_playlist_time(session, pos);
	time_elapsed = xmms_remote_get_output_time(session);
	xmms_remote_get_info(session, &rate, &freq, &chan);
	length = xmms_remote_get_playlist_length(session);
	volume = xmms_remote_get_main_volume(session);
	song = xmms_remote_get_playlist_title(session, pos);

	str = g_string_new("");
	format = purple_prefs_get_string("/plugins/gtk/plugin_pack/xmms-remote/format");

	while(format) {
		if(format[0] != '%') {
			str = g_string_append_c(str, format[0]);
			format++;
			continue;
		}

		format++;
		if(!format[0])
			break;

		switch(format[0]) {
			case '%':
				str = g_string_append_c(str, '%');
				break;
			case 'C':
				g_string_append_printf(str, "%d", chan);
				break;
			case 'F':
				g_string_append_printf(str, "%g", (gfloat)freq / 1000.0f);
				break;
			case 'f':
				g_string_append_printf(str, "%d", freq);
				break;
			case 'L':
				g_string_append_printf(str, "%d", length);
				break;
			case 'P':
				g_string_append_printf(str, "%d", pos + 1);
				break;
			case 'B':
				g_string_append_printf(str, "%g", (gfloat)rate / 1000.0f);
				break;
			case 'b':
				g_string_append_printf(str, "%d", rate);
				break;
			case 'T':
				str = g_string_append(str, song);
				break;
			case 'V':
				g_string_append_printf(str, "%d", volume);
				break;
			case 't':
				min = (gint)(time_total / 60000);
				sec = (gint)(time_total / 1000 % 60);
				g_string_append_printf(str, "%d:%02d", min, sec);
				break;
			case 'e':
				min = (gint)(time_elapsed / 60000);
				sec = (gint)(time_elapsed / 1000 % 60);
				g_string_append_printf(str, "%d:%02d", min, sec);
				break;
			case 'r':
				min = (gint)((time_total - time_elapsed) / 60000);
				sec = (gint)((time_total - time_elapsed) / 1000 % 60);
				g_string_append_printf(str, "%d:%02d", min, sec);
				break;
		}

		format++;
	}

	ret = str->str;
	g_string_free(str, FALSE);

	if(song)
		g_free(song);

	return ret;
}

static void
gxr_display_title(PidginWindow *win) {
	PurpleConversationType type;
	gchar *text = NULL;
	PurpleConversation *conv;

	g_return_if_fail(win);

	conv = pidgin_conv_window_get_active_conversation(win);

	type = purple_conversation_get_type(conv);

	text = gxr_format_info();

	if(!text)
		return;

	switch(type) {
		case PURPLE_CONV_TYPE_IM:
			purple_conv_im_send(PURPLE_CONV_IM(conv), text);
			break;
		case PURPLE_CONV_TYPE_CHAT:
			purple_conv_chat_send(PURPLE_CONV_CHAT(conv), text);
			break;
		case PURPLE_CONV_TYPE_UNKNOWN:
		case PURPLE_CONV_TYPE_MISC:
		default:
			break;
	}

	if(text)
		g_free(text);
}

static void
gxr_menu_display_title_cb(GtkMenuItem *item, gpointer data) {
	PidginWindow *win;

	win = (PidginWindow *)data;

	gxr_display_title(win);
}

static GtkWidget *
gxr_make_item(GtkWidget *menu, const gchar *text, GtkSignalFunc sf,
			  gpointer data)
{
	GtkWidget *item;

	item = gtk_menu_item_new_with_label(text);
	if(menu != NULL)
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_show(item);

	if(sf != NULL)
		g_signal_connect(G_OBJECT(item), "activate", sf, data);

	return item;
}

static GtkWidget *
gxr_make_button(const char *stock, GCallback cb, gpointer data,
				PidginWindow *win)
{
	GtkWidget *ebox, *image;

	ebox = gtk_event_box_new();
	gtk_widget_show(ebox);

	image = gtk_image_new_from_stock(stock, GTK_ICON_SIZE_MENU);
	gtk_container_add(GTK_CONTAINER(ebox), image);
	gtk_widget_show(image);

	g_signal_connect(G_OBJECT(ebox), "button_press_event", G_CALLBACK(cb), data);

	g_object_set_data(G_OBJECT(ebox), "win", win);

	return ebox;
}

static void
gxr_make_playlist(GtkWidget *menu_item) {
	GtkWidget *menu, *item;
	gint i, count, current;
	gint session = GXR_SESSION;
	gchar *title, *song;

	menu = gtk_menu_new();

	count = xmms_remote_get_playlist_length(session);
	current = xmms_remote_get_playlist_pos(session);

	for(i = 0; i < count; i++) {
		song = xmms_remote_get_playlist_title(session, i);
		title = g_strdup_printf("%d. %s", i + 1, song);
		g_free(song);

		if(i == current)
			pidgin_new_check_item(menu, title, G_CALLBACK(gxr_menu_playlist_cb),
								GINT_TO_POINTER(i), TRUE);
		else
			item = gxr_make_item(menu, title, G_CALLBACK(gxr_menu_playlist_cb),
								 GINT_TO_POINTER(i));

		g_free(title);
	}

	gtk_widget_show_all(menu);

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	if(count == 0)
		gtk_widget_set_sensitive(menu_item, FALSE);
}

static GtkWidget *
gxr_make_menu(PidginWindow *win) {
	GtkWidget *menu, *item;
	gint session = GXR_SESSION;

	menu = gtk_menu_new();

	if(!xmms_remote_is_running(session)) {
		item = pidgin_new_item_from_stock(menu, _("Please start XMMS"),
										GXR_STOCK_XMMS, NULL, NULL, 0, 0, NULL);
		gtk_widget_set_sensitive(item, FALSE);

		return menu;
	}

	/* play */
	item = pidgin_new_item_from_stock(menu, _("Play"), GXR_STOCK_PLAY,
									G_CALLBACK(gxr_menu_play_cb), NULL, 0,
									0, NULL);
	if(xmms_remote_is_playing(session) && !xmms_remote_is_paused(session))
		gtk_widget_set_sensitive(item, FALSE);

	/* pause */
	item = pidgin_new_item_from_stock(menu, _("Pause"), GXR_STOCK_PAUSE,
									G_CALLBACK(gxr_menu_pause_cb), NULL, 0,
									0, NULL);
	if(!xmms_remote_is_playing(session) && !xmms_remote_is_paused(session))
		gtk_widget_set_sensitive(item, FALSE);
	if(xmms_remote_is_paused(session))
		gtk_widget_set_sensitive(item, FALSE);

	/* stop */
	item = pidgin_new_item_from_stock(menu, _("Stop"), GXR_STOCK_STOP,
									G_CALLBACK(gxr_menu_stop_cb), NULL, 0,
									0, NULL);
	if(!xmms_remote_is_playing(session) && !xmms_remote_is_paused(session))
		gtk_widget_set_sensitive(item, FALSE);

	/* next */
	pidgin_new_item_from_stock(menu, _("Next"), GXR_STOCK_NEXT,
							 G_CALLBACK(gxr_menu_next_cb), NULL, 0, 0,
							 NULL);

	/* previous */
	pidgin_new_item_from_stock(menu, _("Previous"), GXR_STOCK_PREVIOUS,
							 G_CALLBACK(gxr_menu_prev_cb), NULL, 0, 0,
							 NULL);

	/* separator */
	pidgin_separator(menu);

	/* repeat */
	pidgin_new_check_item(menu, _("Repeat"), G_CALLBACK(gxr_menu_repeat_cb),
						NULL, xmms_remote_is_repeat(session));

	/* shuffle */
	pidgin_new_check_item(menu, _("Shuffle"), G_CALLBACK(gxr_menu_shuffle_cb),
						NULL, xmms_remote_is_shuffle(session));

	if(purple_prefs_get_bool("/plugins/gtk/plugin_pack/xmms-remote/show_playlist")) {
		/* separator */
		pidgin_separator(menu);

		/* playlist */
		item = gxr_make_item(menu, _("Playlist"), NULL, NULL);
		gxr_make_playlist(item);
	}

	if(win) {
		/* Only if we are in a conversation window */
		/* separator */
		pidgin_separator(menu);

		/* title */
		item = gxr_make_item(menu, _("Display title"),
							 G_CALLBACK(gxr_menu_display_title_cb),
							 (gpointer)win);
	}

	return menu;
}

static void
gxr_button_clicked_cb(GtkButton *button, gpointer data) {
	PidginWindow *win;
	GtkWidget *menu;

	win = g_object_get_data(G_OBJECT(button), "win");
	menu = gxr_make_menu(win);

	if(win)
		gtk_widget_grab_focus(pidgin_conv_window_get_active_gtkconv(win)->entry);

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0,
				   gtk_get_current_event_time());
}

static void
gxr_add_button(PidginWindow *win) {
	PidginConversation *gtkconv = pidgin_conv_window_get_active_gtkconv(win);
	PurpleConversation *conv = gtkconv->active_conv;
	PurpleConversationType type = purple_conversation_get_type(conv);
	GtkWidget *tray;
	GtkWidget *button = NULL;
	GList *l;

	if(type != PURPLE_CONV_TYPE_IM && type != PURPLE_CONV_TYPE_CHAT)
		return;

	if(!purple_prefs_get_bool("/plugins/gtk/plugin_pack/xmms-remote/conv"))
		return;

	for(l = buttons; l; l = l->next) {
		button = l->data;
		if(g_object_get_data(G_OBJECT(button), "win") == win)
			return;
	}

	tray = win->menu.tray;

	if(!purple_prefs_get_bool("/plugins/gtk/plugin_pack/xmms-remote/extended")) {
		/* Show the minimal control */
		button = gxr_make_button(GXR_STOCK_XMMS, G_CALLBACK(gxr_button_clicked_cb), win, win);
		pidgin_menu_tray_append(PIDGIN_MENU_TRAY(win->menu.tray), button,
								_("XMMS Remote Control Options"));
		buttons = g_list_append(buttons, (gpointer)button);
	} else {
		/* Show extended control */

		/* we're using gtk_menu_shell_append, so these need to be added in
		 * reverse order
		 */
		button = gxr_make_button(GXR_STOCK_NEXT,
								 G_CALLBACK(gxr_button_next_cb), NULL, win);
		pidgin_menu_tray_append(PIDGIN_MENU_TRAY(win->menu.tray), button, _("Next"));
		buttons = g_list_append(buttons, (gpointer)button);
		
		button = gxr_make_button(GXR_STOCK_STOP,
								 G_CALLBACK(gxr_button_stop_cb), NULL, win);
		pidgin_menu_tray_append(PIDGIN_MENU_TRAY(win->menu.tray), button, _("Stop"));
		buttons = g_list_append(buttons, (gpointer)button);

		button = gxr_make_button(GXR_STOCK_PAUSE,
								 G_CALLBACK(gxr_button_pause_cb), NULL, win);
		pidgin_menu_tray_append(PIDGIN_MENU_TRAY(win->menu.tray), button, _("Pause"));
		buttons = g_list_append(buttons, (gpointer)button);

		button = gxr_make_button(GXR_STOCK_PLAY,
								 G_CALLBACK(gxr_button_play_cb), NULL, win);
		pidgin_menu_tray_append(PIDGIN_MENU_TRAY(win->menu.tray), button, _("Play"));
		buttons = g_list_append(buttons, (gpointer)button);

		button = gxr_make_button(GXR_STOCK_PREVIOUS,
								 G_CALLBACK(gxr_button_prev_cb), NULL, win);
		pidgin_menu_tray_append(PIDGIN_MENU_TRAY(win->menu.tray), button, _("Previous"));
		buttons = g_list_append(buttons, (gpointer)button);

		if(purple_prefs_get_bool("/plugins/gtk/plugin_pack/xmms-remote/volume")) {
			/* The volume controller */
			GtkWidget *slider;

			slider = gtk_hscale_new_with_range(0, 100, 1);
			gtk_widget_set_usize(GTK_WIDGET(slider), 100, -1);
			gtk_scale_set_draw_value(GTK_SCALE(slider), FALSE);
			gtk_range_set_value(GTK_RANGE(slider),
								xmms_remote_get_main_volume(GXR_SESSION));
			pidgin_menu_tray_append(PIDGIN_MENU_TRAY(win->menu.tray),
									slider, _("XMMS Volume Control"));

			g_object_set_data(G_OBJECT(slider), "win", win);
			buttons = g_list_prepend(buttons, slider);

			g_signal_connect(G_OBJECT(slider), "value-changed",
							G_CALLBACK(gxr_update_volume), NULL);
			gtk_widget_show(slider);

		}
	}
}

static void
gxr_show_buttons(void) {
	GList *wins;

	for(wins = pidgin_conv_windows_get_list(); wins; wins = wins->next)
		gxr_add_button(wins->data);
}

static void
gxr_hide_buttons(void) {
	PidginWindow *win;
	GtkWidget *button;
	GList *l, *l_next;

	for(l = buttons; l; l = l_next) {
		l_next = l->next;
		button = GTK_WIDGET(l->data);
		win = g_object_get_data(G_OBJECT(button), "win");

		if(!win) {
			buttons = g_list_remove(buttons, button);
		} else {
			gtk_widget_destroy(button);
			buttons = g_list_remove(buttons, button);
		}
	}
}

static gboolean
is_active(GList *list, gint id) {
	for (; list; list = list->next) {
		int itmp = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(list->data), "gxr-id"));

		if(itmp == id)
			return (GTK_WIDGET_IS_SENSITIVE(GTK_WIDGET(list->data)) && 
						gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(list->data)));
	}

	return FALSE;
}

static void
set_active(GList *list, gint id, gboolean active) {
	for (; list; list = list->next) {
		int itmp = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(list->data), "gxr-id"));

		if(itmp == id) {
			gtk_widget_set_sensitive(GTK_WIDGET(list->data), active);
			break;
		}
	}
}

static void refresh_buttons() {
	if(is_active(checkboxes, GXR_PREF_CONV_WINDOW)) {
		set_active(checkboxes, GXR_PREF_EXTENDED_CTRL, TRUE);
		
		if(is_active(checkboxes, GXR_PREF_EXTENDED_CTRL))
			set_active(checkboxes, GXR_PREF_VOLUME_CTRL, TRUE);
		else
			set_active(checkboxes, GXR_PREF_VOLUME_CTRL, FALSE);
	} else {
		set_active(checkboxes, GXR_PREF_EXTENDED_CTRL, FALSE);
		set_active(checkboxes, GXR_PREF_VOLUME_CTRL, FALSE);
	}
}

static void
gxr_button_show_cb(const char *name, PurplePrefType type, gconstpointer val,
				   gpointer data)
{
	refresh_buttons();

	gxr_hide_buttons();
	gxr_show_buttons();
}

static void
gxr_popup_cb(GtkWidget *w, GtkMenu *menu, gpointer data) {
	GtkWidget *item, *submenu;

	pidgin_separator(GTK_WIDGET(menu));

	item = pidgin_new_item_from_stock(GTK_WIDGET(menu), _("XMMS Remote Control"),
									GXR_STOCK_XMMS, NULL, NULL, 0, 0, NULL);

	submenu = gxr_make_menu((PidginWindow *)data);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

	gtk_widget_show(item);
}

void
gxr_disconnect_popup_cb(PurpleConversation *conv) {
	PidginWindow *win;
	PidginConversation *gtkconv;
	GtkWidget *entry;
	gulong handle;

	if((gtkconv = PIDGIN_CONVERSATION(conv)) == NULL)
		return;

	win = pidgin_conv_get_window(gtkconv);
	entry = gtkconv->entry;

	handle = (gulong)g_object_get_data(G_OBJECT(entry), "gxr-popup-handle");
	if(handle == 0)
		return;

	g_signal_handler_disconnect(G_OBJECT(entry), handle);
	g_object_set_data(G_OBJECT(entry), "gxr-popup-handle", 0);
}

static void
gxr_hook_popup_for_gtkconv(PidginConversation *gtkconv) {
	gulong handle;
	GtkWidget *entry;
	PidginWindow *win;

	win = pidgin_conv_get_window(gtkconv);
	entry = gtkconv->entry;

	if ((gulong)g_object_get_data(G_OBJECT(entry), "gxr-popup-handle"))
		return;

	handle = g_signal_connect(G_OBJECT(entry), "populate-popup",
							  G_CALLBACK(gxr_popup_cb), win);
	g_object_set_data(G_OBJECT(entry), "gxr-popup-handle",
							   (gpointer)handle);
}

static void
gxr_hook_popups(void) {
	GList *convs;
	PurpleConversation *conv;

	for(convs = purple_get_conversations(); convs; convs = convs->next) {
		PidginConversation *gtkconv;

		conv = (PurpleConversation *)convs->data;
		gtkconv = PIDGIN_CONVERSATION(conv);
		gxr_hook_popup_for_gtkconv(gtkconv);
	}
}

static gboolean
attach_to_window_tray(PurpleConversation *conv) {
	PidginConversation *gtkconv;
	PidginWindow *win;

	if((gtkconv = PIDGIN_CONVERSATION(conv)) == NULL)
		return TRUE;

	win = pidgin_conv_get_window(gtkconv);
	if(!win)
		return TRUE;

	if(!win->window)
		return TRUE;

	if(!GTK_WIDGET_VISIBLE(win->window))
		return TRUE;

	gxr_add_button(win);
	gxr_hook_popup_for_gtkconv(gtkconv);

	return FALSE;
}

static void
gxr_conv_created_cb(PurpleConversation *conv, gpointer data) {
	/* This terrible workaround is necessary because when you have
	 * message-queueing enabled, the conversation does NOT get attached
	 * to the viewable window immediately when it gets created. Rather,
	 * it is shown only after the signal has been emitted. There might
	 * be a better way of doing this. */
	g_timeout_add(50, (GSourceFunc)attach_to_window_tray, conv);
}

static void
gxr_conv_destroyed_cb(PurpleConversation *conv, gpointer data) {
	PidginWindow *win;
	GtkWidget *button;
	GList *l, *l_next;

	win = pidgin_conv_get_window(PIDGIN_CONVERSATION(conv));

	if(win == NULL || pidgin_conv_window_get_gtkconv_count(win) != 1)
		return;

	for(l = buttons; l != NULL; l = l_next) {
		l_next = l->next;

		button = GTK_WIDGET(l->data);

		if(win == g_object_get_data(G_OBJECT(button), "win")) {
			gtk_widget_destroy(button);
			buttons = g_list_remove(buttons, l->data);
		}
	}
}

static PurpleCmdRet
gxr_cmd_cb(PurpleConversation *c, const gchar *cmd, gchar **args, gchar **error,
		void *data)
{
	gchar *lower;
	gint session = GXR_SESSION;
	PidginWindow *win;

	win = pidgin_conv_get_window(PIDGIN_CONVERSATION(c));

	if(!xmms_remote_is_running(session)) {
		*error = g_strdup(_("XMMS is not running"));
		return PURPLE_CMD_RET_FAILED;
	}

	if(!*args && !args[0]) {
		*error = g_strdup("eek!");
		return PURPLE_CMD_RET_FAILED;
	}

	lower = g_ascii_strdown(args[0], strlen(args[0]));

	if(strcmp(lower, "play") == 0)
		xmms_remote_play(session);
	else if(!strcmp(lower, "pause"))
		xmms_remote_pause(session);
	else if(!strcmp(lower, "stop"))
		xmms_remote_stop(session);
	else if(!strcmp(lower, "next"))
		xmms_remote_playlist_next(session);
	else if(!strcmp(lower, "prev"))
		xmms_remote_playlist_prev(session);
	else if(!strcmp(lower, "info"))
		gxr_display_title(win);
	else if(!strcmp(lower, "repeat"))
		xmms_remote_toggle_repeat(session);
	else if(!strcmp(lower, "shuffle"))
		xmms_remote_toggle_shuffle(session);
	else if(!strcmp(lower, "show"))
		xmms_remote_main_win_toggle(session, TRUE);
	else if(!strcmp(lower, "hide"))
		xmms_remote_main_win_toggle(session, FALSE);
	else {
		*error = g_strdup(_("unknown argument"));
		return PURPLE_CMD_RET_FAILED;
	}

	g_free(lower);

	return PURPLE_CMD_RET_OK;
}

/*******************************************************************************
 * Prefs stuff
 ******************************************************************************/
static GtkWidget *
gxr_make_label(const gchar *text, GtkSizeGroup *sg) {
	GtkWidget *label;

	label = gtk_label_new_with_mnemonic(text);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);

	if(sg)
		gtk_size_group_add_widget(sg, label);

	return label;
}

static GtkWidget *
gxr_get_config_frame(PurplePlugin *plugin) {
	GtkWidget *vbox, *hbox, *frame, *label, *checkbox;
	GtkSizeGroup *sg;

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);

	frame = pidgin_make_frame(vbox, _("Info"));

	pidgin_prefs_labeled_entry(frame, _("Info Format:"),
								 "/plugins/gtk/plugin_pack/xmms-remote/format",
								 NULL);

	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(frame), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	label = gxr_make_label(_("%T: Song title"), sg);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	label = gxr_make_label(_("%C: Number of channels"), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(frame), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	label = gxr_make_label(_("%P: Current song playlist number"), sg);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	label = gxr_make_label(_("%L: Total songs in the playlist"), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(frame), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	label = gxr_make_label(_("%t: Total time"), sg);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	label = gxr_make_label(_("%e: Elapsed time"), sg);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(frame), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	label = gxr_make_label(_("%r: Remaining time"), sg);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	label = gxr_make_label(_("%V: Current volume"), sg);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(frame), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);
	
	label = gxr_make_label(_("%f: Frequency in Hz"), sg);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	label = gxr_make_label(_("%F: Frequency in kHz"), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(frame), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	label = gxr_make_label(_("%b: Bitrate in bps"), sg);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	label = gxr_make_label(_("%B: Bitrate in kBps"), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	frame = pidgin_make_frame(vbox, _("Appearance"));

	checkbox = pidgin_prefs_checkbox(_("Show playlist in the control menu"),
							"/plugins/gtk/plugin_pack/xmms-remote/show_playlist",
							frame);
	g_object_set_data(G_OBJECT(checkbox), "gxr-id", GINT_TO_POINTER(GXR_PREF_PLAYLIST));
	checkboxes = g_list_prepend(checkboxes, checkbox);

	checkbox = pidgin_prefs_checkbox(_("Show controls in buddy list"),
							"/plugins/gtk/plugin_pack/xmms-remote/blist",
							frame);
	g_object_set_data(G_OBJECT(checkbox), "gxr-id", GINT_TO_POINTER(GXR_PREF_BLIST));
	checkboxes = g_list_prepend(checkboxes, checkbox);
	
	checkbox = pidgin_prefs_checkbox(_("Show controls in conversation windows"),
							"/plugins/gtk/plugin_pack/xmms-remote/conv",
							frame);
	g_object_set_data(G_OBJECT(checkbox), "gxr-id", GINT_TO_POINTER(GXR_PREF_CONV_WINDOW));
	checkboxes = g_list_prepend(checkboxes, checkbox);

	checkbox = pidgin_prefs_checkbox(_("Show extended controls (Conversation windows only)"),
							"/plugins/gtk/plugin_pack/xmms-remote/extended",
							frame);
	g_object_set_data(G_OBJECT(checkbox), "gxr-id", GINT_TO_POINTER(GXR_PREF_EXTENDED_CTRL));
	checkboxes = g_list_prepend(checkboxes, checkbox);

	checkbox = pidgin_prefs_checkbox(_("Show volume control (Conversation windows only)"),
							"/plugins/gtk/plugin_pack/xmms-remote/volume",
							frame);
	g_object_set_data(G_OBJECT(checkbox), "gxr-id", GINT_TO_POINTER(GXR_PREF_VOLUME_CTRL));
	checkboxes = g_list_prepend(checkboxes, checkbox);

	frame = pidgin_make_frame(vbox, _("Advanced"));

	pidgin_prefs_labeled_spin_button(frame, _("XMMS instance to control"),
								   "/plugins/gtk/plugin_pack/xmms-remote/session",
								   0, 65535, NULL);

	refresh_buttons();
	
	gtk_widget_show_all(vbox);

	return vbox;
}

/*******************************************************************************
 * Stock stuff
 ******************************************************************************/
static gchar *
gxr_file_name(const gchar *file_name) {
	return g_build_filename(PIDGIN_PIXMAPSDIR, "plugin_pack",
			"xmmsremote", file_name, NULL);
}

static const GtkStockItem stock_items[] =
{
	{ GXR_STOCK_XMMS,     "",     0, 0, NULL },
	{ GXR_STOCK_PREVIOUS, "",     0, 0, NULL },
	{ GXR_STOCK_PLAY,     "",     0, 0, NULL },
	{ GXR_STOCK_PAUSE,    "",     0, 0, NULL },
	{ GXR_STOCK_STOP,     "",     0, 0, NULL },
	{ GXR_STOCK_NEXT,     "",     0, 0, NULL }
};

static void
gxr_add_to_stock(const gchar *file_name, const gchar *stock_name) {
	GtkIconSet *icon_set;
	GdkPixbuf *pixbuf;
	gchar *file = gxr_file_name(file_name);

	pixbuf = gdk_pixbuf_new_from_file(file, NULL);
	g_free(file);

	icon_set = gtk_icon_set_new_from_pixbuf(pixbuf);
	g_object_unref(G_OBJECT(pixbuf));

	gtk_icon_factory_add(icon_factory, stock_name, icon_set);
	gtk_icon_set_unref(icon_set);
}

static void
gxr_init_stock() {
	icon_factory = gtk_icon_factory_new();
	gtk_icon_factory_add_default(icon_factory);

	gxr_add_to_stock("next.png", GXR_STOCK_NEXT);
	gxr_add_to_stock("pause.png", GXR_STOCK_PAUSE);
	gxr_add_to_stock("play.png", GXR_STOCK_PLAY);
	gxr_add_to_stock("previous.png", GXR_STOCK_PREVIOUS);
	gxr_add_to_stock("stop.png", GXR_STOCK_STOP);
	gxr_add_to_stock("xmms.png", GXR_STOCK_XMMS);

	gtk_stock_add_static(stock_items, G_N_ELEMENTS(stock_items));
}

static void
gxr_hook_blist(const char *name, PurplePrefType type, gconstpointer val,
				gpointer data)
{
	gboolean value = GPOINTER_TO_INT(val);

	if(value && !blist_button) {
		PidginBuddyList *gtkblist = pidgin_blist_get_default_gtk_blist();

		blist_button = gxr_make_button(GXR_STOCK_XMMS,
								G_CALLBACK(gxr_button_clicked_cb), NULL, NULL);
		pidgin_menu_tray_append(PIDGIN_MENU_TRAY(gtkblist->menutray),
								blist_button, _("XMMS Remote Control Options"));
	} else {
		if(blist_button) {
			gtk_widget_destroy(blist_button);
			blist_button = NULL;
		}
	}
}

static void
gxr_gtkblist_created_cb(void) {
	purple_prefs_trigger_callback("/plugins/gtk/plugin_pack/xmms-remote/blist");
}

/*******************************************************************************
 * Plugin stuff
 ******************************************************************************/

static gboolean
gxr_load(PurplePlugin *plugin) {
	void *conv_handle = NULL;

	/* XXX: Translators: leave "xmms" and the argument names untranslated */
	const gchar *help = _("<pre>xmms &lt;[play][pause][stop][next][prev][repeat][shuffle][show][hide][info]&gt;\n"
						"play     Starts playback\n"
						"pause    Pauses playback\n"
						"stop     Stops playback\n"
						"next     Goes to the next song in the playlist\n"
						"prev     Goes to the previous song in the playlist\n"
						"repeat   Toggles repeat\n"
						"shuffle  Toggles shuffling\n"
						"show     Show the XMMS window\n"
						"hide     Hide the XMMS window\n"
						"info     Displays currently playing song in the conversation\n</pre>");

	conv_handle = purple_conversations_get_handle();
	/* init our stock */
	gxr_init_stock();

	/* connect to the signals we need.. */
	purple_signal_connect(conv_handle, "conversation-created", plugin,
						PURPLE_CALLBACK(gxr_conv_created_cb), NULL);

	purple_signal_connect(conv_handle, "deleting-conversation", plugin,
						PURPLE_CALLBACK(gxr_conv_destroyed_cb), NULL);

	purple_prefs_connect_callback(plugin, "/plugins/gtk/plugin_pack/xmms-remote/conv",
								gxr_button_show_cb, NULL);

	purple_prefs_connect_callback(plugin, "/plugins/gtk/plugin_pack/xmms-remote/blist",
								gxr_hook_blist, NULL);

	purple_prefs_connect_callback(plugin, "/plugins/gtk/plugin_pack/xmms-remote/extended",
								gxr_button_show_cb, NULL);

	purple_prefs_connect_callback(plugin, "/plugins/gtk/plugin_pack/xmms-remote/volume",
								gxr_button_show_cb, NULL);

	gxr_show_buttons();

	/* register our command */
	gxr_cmd = purple_cmd_register("xmms", "w", PURPLE_CMD_P_PLUGIN,
								PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT, NULL,
								gxr_cmd_cb, help, NULL);

	/* hijack the popup menus in the imhtmls */
	gxr_hook_popups();

	if(purple_prefs_get_bool("/plugins/gtk/plugin_pack/xmms-remote/blist") &&
			!pidgin_blist_get_default_gtk_blist())
	{
		purple_signal_connect(pidgin_blist_get_handle(), "gtkblist-created", 
					plugin, PURPLE_CALLBACK(gxr_gtkblist_created_cb), NULL);
	} else
		purple_prefs_trigger_callback("/plugins/gtk/plugin_pack/xmms-remote/blist");

	return TRUE;
}

static gboolean
gxr_unload(PurplePlugin *plugin) {
	/* remove our buttons */
	gxr_hide_buttons();

	g_list_free(buttons);
	buttons = NULL;

	if(blist_button) {
		gtk_widget_destroy(blist_button);
		blist_button = NULL;
	}

	/* remove our popup menu item */
	purple_conversation_foreach(gxr_disconnect_popup_cb);
	/* remove our icons */
	gtk_icon_factory_remove_default(icon_factory);

	/* remove our command */
	purple_cmd_unregister(gxr_cmd);

	return TRUE;
}

static PidginPluginUiInfo ui_info = {
	gxr_get_config_frame,
	0,
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginInfo gxr_info = {
	PURPLE_PLUGIN_MAGIC,									/* Don't			*/
	PURPLE_MAJOR_VERSION,									/* fear the		*/
	PURPLE_MINOR_VERSION,									/* reaper		*/
	PURPLE_PLUGIN_STANDARD,									/* type			*/
	PIDGIN_PLUGIN_TYPE,										/* ui requirement	*/
	0,														/* flags			*/
	NULL,													/* dependencies	*/
	PURPLE_PRIORITY_DEFAULT,								/* priority		*/

	"gtk-plugin_pack-xmmsremote",							/* id			*/
	NULL,													/* name			*/
	PP_VERSION,												/* version		*/
	NULL,													/* summary		*/
	NULL,													/* description	*/

	"Gary Kramlich <grim@reaperworld.com>",					/* author		*/
	PP_WEBSITE,												/* homepage		*/

	gxr_load,												/* load			*/
	gxr_unload,												/* unload		*/
	NULL,													/* destroy		*/

	&ui_info,												/* ui info		*/
	NULL,													/* extra info		*/
	NULL,													/* prefs info		*/
	NULL,													/* actions info	*/
	NULL,													/* reserved 1	*/
	NULL,													/* reserved 2	*/
	NULL,													/* reserved 3	*/
	NULL													/* reserved 4	*/
};
static void
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	gxr_info.name = _("XMMS Remote Control");
	gxr_info.summary = _("Control XMMS from Pidgin conversations");
	gxr_info.description = _("A small plugin that adds a menu or buttons to the menu"
		" bars of Pidgin conversation windows so that you can control XMMS from within Pidgin.");

	purple_prefs_add_none("/plugins/gtk/plugin_pack");
	purple_prefs_add_none("/plugins/gtk/plugin_pack/xmms-remote");

	/* XXX: Translators: Please leave the %T in this string.  It is used internally.  */
	purple_prefs_add_string("/plugins/gtk/plugin_pack/xmms-remote/format",
						  _("/me is listening to %T"));
	purple_prefs_add_int("/plugins/gtk/plugin_pack/xmms-remote/session", 0);
	purple_prefs_add_bool("/plugins/gtk/plugin_pack/xmms-remote/show_playlist", TRUE);
	purple_prefs_add_bool("/plugins/gtk/plugin_pack/xmms-remote/conv", TRUE);
	purple_prefs_add_bool("/plugins/gtk/plugin_pack/xmms-remote/blist", TRUE);
	purple_prefs_add_bool("/plugins/gtk/plugin_pack/xmms-remote/extended", TRUE);
	purple_prefs_add_bool("/plugins/gtk/plugin_pack/xmms-remote/volume", TRUE);
}

PURPLE_INIT_PLUGIN(xmmsremote, init_plugin, gxr_info)

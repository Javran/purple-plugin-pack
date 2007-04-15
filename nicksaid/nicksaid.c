/*
 * Nicksaid - Record when someone said your nick in a chat.
 * Copyright (C) 2006
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

#define PLUGIN_ID			"gtk-plugin_pack-nicksaid"
#define PLUGIN_NAME			"Nicksaid"
#define PLUGIN_STATIC_NAME	"Nicksaid"
#define PLUGIN_SUMMARY		"Record when someone said your nick in a chat."
#define PLUGIN_DESCRIPTION	"Record when someone said your nick in a chat."
#define PLUGIN_AUTHOR		"Sadrul H Chowdhury <sadrul@users.sourceforge.net>"

#define PREF_PREFIX			"/plugins/gtk/" PLUGIN_ID
#define PREF_HLWORDS		PREF_PREFIX "/hlwords"
#define PREF_CHARS			PREF_PREFIX "/chars"
#define PREF_TIMESTAMP		PREF_PREFIX "/timestamp"
#define PREF_DATESTAMP		PREF_PREFIX "/datestamp"
#define PREF_SHOWWHO		PREF_PREFIX "/showwho"
#define PREF_SHOWALL		PREF_PREFIX "/showall"

/* System headers */
#include <string.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>

/* Purple headers */
#include <gtkplugin.h>
#include <version.h>

#include <conversation.h>
#include <notify.h>

#include <gtkconv.h>
#include <gtkimhtml.h>
#include <gtkmenutray.h>
#include <pidginstock.h>
#include <gtkutils.h>

/* Pack/Local headers */
#include "../common/i18n.h"

#define DELIMS " .,;|<>?/\\`~!@#$%^&*()_-+={}[]:'\""

static GList *hlwords = NULL;		/* Words to highlight on */

#if 0
static void
go_next(GtkWidget *w, PidginConversation *gtkconv)
{
}

static void
go_previous(GtkWidget *w, PidginConversation *gtkconv)
{
}
#endif

typedef struct _NickSaid NickSaid;

struct _NickSaid
{
	int offset;
	char *who;
	char *what;
};

/* <lift src="gaim/src/util.c"> */
static const gchar *
ns_time(void)
{
	static gchar buf[80];
	time_t tme;

	time(&tme);
	strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&tme));

	return buf;
}

static const gchar *
ns_date(void)
{
	static gchar buf[80];
	time_t tme;

	time(&tme);
	strftime(buf, sizeof(buf), "%Y-%m-%d", localtime(&tme));

	return buf;
}

static const gchar *
ns_date_full(void)
{
	gchar *buf;
	time_t tme;

	time(&tme);
	buf = ctime(&tme);
	buf[strlen(buf) - 1] = '\0';

	return buf;
}
/* <lift/> */

struct _callbackdata
{
	GtkTextView *view;
	GtkTextIter iter;
};

static gboolean draw_line(GtkWidget *widget, GdkEventExpose *event, struct _callbackdata *data);

static gboolean
remove_line(struct _callbackdata *data)
{
	if (g_signal_handlers_disconnect_matched(G_OBJECT(data->view), G_SIGNAL_MATCH_DATA,
				0, 0, NULL, NULL, data))
	{
		g_object_set_data(G_OBJECT(data->view), "nicksaid:mark", GINT_TO_POINTER(0));
		gtk_widget_queue_draw(GTK_WIDGET(data->view));
		g_free(data);
	}

	return FALSE;
}

static gboolean
draw_line(GtkWidget *widget, GdkEventExpose *event, struct _callbackdata *data)
{
	GtkTextIter iter;
	GdkRectangle rect, visible_rect;
	int top, bottom, pad;
	GdkGC *gc;
	GtkTextView *view;
	GdkColor green = {0, 0, 0xffff, 0};

	view = data->view;
	iter = data->iter;

	gtk_text_view_get_iter_location(view, &iter, &rect);
	pad = (gtk_text_view_get_pixels_below_lines(view) + 
			gtk_text_view_get_pixels_above_lines(view)) / 2;
	top = rect.y - pad;
	bottom = rect.y + rect.height + pad;

	gtk_text_view_buffer_to_window_coords(view, GTK_TEXT_WINDOW_TEXT,
										0, top, 0, &top);
	gtk_text_view_buffer_to_window_coords(view, GTK_TEXT_WINDOW_TEXT,
										0, bottom, 0, &bottom);

	gtk_text_view_get_visible_rect(view, &visible_rect);

	gc = gdk_gc_new(GDK_DRAWABLE(event->window));
	gdk_gc_set_rgb_fg_color(gc, &green);
	gdk_draw_line(event->window, gc, 0, top, visible_rect.width, top);
	gdk_draw_line(event->window, gc, 0, bottom, visible_rect.width, bottom);
	gdk_gc_unref(gc);

	if (!g_object_get_data(G_OBJECT(view), "nicksaid:mark"))
	{
		g_timeout_add(2500, (GSourceFunc)remove_line, data);
		g_object_set_data(G_OBJECT(view), "nicksaid:mark", GINT_TO_POINTER(1));
	}

	return FALSE;
}

static void
go_selected(GtkWidget *w, PidginConversation *gtkconv)
{
	GtkTextIter iter;
	int offset;
	struct _callbackdata *data;

	offset = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "nicksaid:offset")) + 1;

	gtk_text_buffer_get_iter_at_offset(GTK_IMHTML(gtkconv->imhtml)->text_buffer, &iter, offset);
	gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(gtkconv->imhtml), &iter, 0, TRUE, 0, 0);

	data = g_new0(struct _callbackdata, 1);
	data->view = GTK_TEXT_VIEW(gtkconv->imhtml);
	data->iter = iter;
	
	g_signal_connect(G_OBJECT(gtkconv->imhtml), "expose_event", G_CALLBACK(draw_line), data);
	gtk_widget_queue_draw(gtkconv->imhtml);
}

static void
clear_list(GtkWidget *w, PidginConversation *gtkconv)
{
	GList *list = g_object_get_data(G_OBJECT(gtkconv->imhtml), "nicksaid:list");

	while (list)
	{
		NickSaid *said = list->data;
		g_free(said->who);
		g_free(said->what);
		g_free(said);
		list = list->next;
	}

	g_object_set_data(G_OBJECT(gtkconv->imhtml), "nicksaid:list", NULL);
}

static void
show_all(GtkWidget *w, PidginConversation *gtkconv)
{
	GList *list = g_object_get_data(G_OBJECT(gtkconv->imhtml), "nicksaid:list");
	GString *str = g_string_new(NULL);

	while (list)
	{
		NickSaid *said = list->data;
		g_string_append_printf(str, "%s\n", said->what);
		list = list->next;
	}

	purple_notify_formatted(gtkconv, _("Nicksaid"), _("List of highlighted messages:"),
			NULL, str->str, NULL, NULL);
	g_string_free(str, TRUE);
}

static gboolean
generate_popup(GtkWidget *w, GdkEventButton *event, PidginWindow *win)
{
	GtkWidget *menu, *item;
	PurpleConversation *conv;
	PidginConversation *gtkconv;
	GList *list;

	conv = pidgin_conv_window_get_active_conversation(win);
	if (purple_conversation_get_type(conv) != PURPLE_CONV_TYPE_CHAT)
		return FALSE;

	menu = gtk_menu_new();

	gtkconv = PIDGIN_CONVERSATION(conv);

	list = g_object_get_data(G_OBJECT(gtkconv->imhtml), "nicksaid:list");
	if (!list)
	{
		item = gtk_menu_item_new_with_label(_("None"));
		gtk_widget_set_sensitive(item, FALSE);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		gtk_widget_show(item);
	}
	else
	{
#if 0
		item = gtk_menu_item_new_with_label(_("Next"));
		gtk_widget_show(item);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(go_next), gtkconv);

		item = gtk_menu_item_new_with_label(_("Previous"));
		gtk_widget_show(item);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(go_previous), gtkconv);

		pidgin_separator(menu);
#endif

		while (list)
		{
			NickSaid *said = list->data;
			item = gtk_menu_item_new_with_label(said->who);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			gtk_widget_show(item);
			g_object_set_data(G_OBJECT(item), "nicksaid:offset",
							GINT_TO_POINTER(said->offset));
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(go_selected), gtkconv);
		/* TODO:
		 * If the line to scrollback to is greater /gaim/gtk/conversations/scrollback_lines,
		 * desensitise the widget so it at least displays.. or something like that. you get the drift. */
			list = list->next;
		}

		pidgin_separator(menu);

		item = gtk_menu_item_new_with_label(_("Clear History"));
		gtk_widget_show(item);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(clear_list), gtkconv);

		item = gtk_menu_item_new_with_label(_("Show All"));
		gtk_widget_show(item);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		if (purple_prefs_get_bool(PREF_SHOWALL))
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(show_all), gtkconv);
		else
			gtk_widget_set_sensitive(item, FALSE);
	}

	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0,
					GDK_CURRENT_TIME);

	return TRUE;
}

static gboolean
displaying_msg_cb(PurpleAccount *account, const char *name, char **buffer,
				PurpleConversation *conv, PurpleMessageFlags flags, gpointer null)
{
	PidginConversation *gtkconv;
	GtkWidget *imhtml;
	GtkTextIter iter;
	GList *list;
	int pos;
	char *tmp, *who, *prefix = NULL;
	NickSaid *said;
	gboolean timestamp = purple_prefs_get_bool(PREF_TIMESTAMP);
	gboolean datestamp = purple_prefs_get_bool(PREF_DATESTAMP);
	gboolean showwho = purple_prefs_get_bool(PREF_SHOWWHO);

	if (!(flags & PURPLE_MESSAGE_NICK) || !PIDGIN_IS_PIDGIN_CONVERSATION(conv) ||
			purple_conversation_get_type(conv) != PURPLE_CONV_TYPE_CHAT)
		return FALSE;

	gtkconv = PIDGIN_CONVERSATION(conv);
	imhtml = gtkconv->imhtml;

	gtk_text_buffer_get_end_iter(GTK_IMHTML(imhtml)->text_buffer, &iter);
	pos = gtk_text_iter_get_offset(&iter);
	list = g_object_get_data(G_OBJECT(imhtml), "nicksaid:list");

	tmp = purple_markup_strip_html(*buffer);
	who = g_strndup(tmp, purple_prefs_get_int(PREF_CHARS));
	g_free(tmp);

	if (!g_utf8_validate(who, -1, (const char **)&tmp))
		*tmp = '\0';

	if (showwho) {
		tmp = who;
		who = g_strdup_printf("%s: %s", name, tmp);
		g_free(tmp);
	}

	if (datestamp && timestamp) {
		prefix = g_strdup_printf("(%s) ", ns_date_full());
	} else if (datestamp && !timestamp) {
		prefix = g_strdup_printf("(%s) ", ns_date());
	} else if (!datestamp && timestamp) {
		prefix = g_strdup_printf("(%s) ", ns_time());
	}

	said = g_new0(NickSaid, 1);
	said->offset = pos;

	if (prefix != NULL) {
		said->who = g_strdup_printf("%s%s", prefix, who);
		g_free(who);
	} else {
		said->who = who;
	}

	if (purple_prefs_get_bool(PREF_SHOWALL))
	{
		said->what = g_strdup_printf("%s<b>%s: </b>%s",
				prefix ? prefix : "", name, *buffer);
	}

	g_free(prefix);

	list = g_list_prepend(list, said);
	g_object_set_data(G_OBJECT(imhtml), "nicksaid:list", list);

	return FALSE;
}

static GtkWidget *
get_tray_icon_for_window(PidginWindow *win)
{
	GtkWidget *w = g_object_get_data(G_OBJECT(win->window), "nicksaid:trayicon");
	if (w == NULL)
	{
		w = gtk_event_box_new();
		gtk_container_add(GTK_CONTAINER(w), 
					gtk_image_new_from_stock(PIDGIN_STOCK_TOOLBAR_MESSAGE_NEW, GTK_ICON_SIZE_MENU));
		pidgin_menu_tray_append(PIDGIN_MENU_TRAY(win->menu.tray), w, "Nicksaid");
		gtk_widget_show_all(w);
		g_object_set_data(G_OBJECT(win->window), "nicksaid:trayicon", w);

		g_signal_connect(G_OBJECT(w), "button_press_event", G_CALLBACK(generate_popup), win);
	}
	return w;
}

static void
update_menu_tray(PurpleConversation *conv, gpointer null)
{
	PidginConversation *gtkconv;
	PidginWindow *win;
	GtkWidget *icon;

	if (!PIDGIN_IS_PIDGIN_CONVERSATION(conv))
		return;

	gtkconv = PIDGIN_CONVERSATION(conv);
	win = gtkconv->win;

	if (!win)
		return;

	icon = get_tray_icon_for_window(win);

	if (purple_conversation_get_type(conv) != PURPLE_CONV_TYPE_CHAT)
		gtk_widget_hide(icon);
	else
		gtk_widget_show(icon);
}

static void
deleting_conversation_cb(PurpleConversation *conv, gpointer null)
{
	if (purple_conversation_get_type(conv) != PURPLE_CONV_TYPE_CHAT ||
			!PIDGIN_IS_PIDGIN_CONVERSATION(conv))
		return;

	clear_list(NULL, PIDGIN_CONVERSATION(conv));
}

static gboolean
rcvd_msg_cb(PurpleAccount *account, const char **who, const char **message,
				PurpleConversation *conv, PurpleMessageFlags *flags)
{
	char *msg, *delims;
	GList *iter;

	if (*flags & PURPLE_MESSAGE_NICK)
		return FALSE;

	delims = g_strdup(DELIMS);
	g_strdelimit(delims, purple_prefs_get_string(PREF_HLWORDS), ' ');
	
	msg = g_strdup(*message);
	g_strdelimit(msg, delims, ' ');
	g_free(delims);

	for (iter = hlwords; iter; iter = iter->next)
	{
		char *s = g_strstr_len(msg, -1, iter->data);
		if (s && (s == msg || *(s - 1) == ' '))
		{
			char *e = s + strlen(iter->data);
			if (*e == ' ' || *e == '\0')
			{
				*flags |= PURPLE_MESSAGE_NICK;
				break;
			}
		}
	}	

	g_free(msg);
	return FALSE;
}

static void
destroy_list()
{
	while (hlwords)
	{
		g_free(hlwords->data);
		hlwords = g_list_delete_link(hlwords, hlwords);
	}
}

static void
construct_list()
{
	char *string;
	const char *s, *e;

	destroy_list();

	string = g_strdup(purple_prefs_get_string(PREF_HLWORDS));
	s = string;
	e = NULL;

	if (s == NULL)
		return;

	while (*s)
	{
		while (*s == ' ' || *s == '\t')
			s++;
		e = s + 1;
		while (*e != ' ' && *e != '\t' && *e != '\0')
			e++;

		hlwords = g_list_prepend(hlwords, g_strndup(s, e-s));
		s = e;
	}

	g_free(string);
}

static void
unload_cleanup_gtkconv(PidginConversation *gtkconv, gpointer null)
{
	clear_list(NULL, gtkconv);
}

static void
unload_cleanup_win(PidginWindow *win, gpointer null)
{
	GtkWidget *w = get_tray_icon_for_window(win);
	g_object_set_data(G_OBJECT(win->window), "nicksaid:trayicon", NULL);
	gtk_widget_destroy(w);

	g_list_foreach(pidgin_conv_window_get_gtkconvs(win), (GFunc)unload_cleanup_gtkconv, NULL);
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	construct_list();

	purple_signal_connect(purple_conversations_get_handle(), "receiving-chat-msg",
						plugin,	PURPLE_CALLBACK(rcvd_msg_cb), NULL);
	purple_signal_connect(pidgin_conversations_get_handle(), "displaying-chat-msg",
						plugin, PURPLE_CALLBACK(displaying_msg_cb), NULL);
	purple_signal_connect(pidgin_conversations_get_handle(), "conversation-switched",
						plugin, PURPLE_CALLBACK(update_menu_tray), NULL);
	purple_signal_connect(purple_conversations_get_handle(), "deleting-conversation",
						plugin, PURPLE_CALLBACK(deleting_conversation_cb), NULL);

	purple_prefs_connect_callback(plugin, PREF_HLWORDS,
					(PurplePrefCallback)construct_list, NULL);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	destroy_list();

	g_list_foreach(pidgin_conv_windows_get_list(), (GFunc)unload_cleanup_win, NULL);

	purple_prefs_disconnect_by_handle(plugin);

	return TRUE;
}

static PurplePluginPrefFrame *
get_plugin_pref_frame(PurplePlugin *plugin)
{
	PurplePluginPrefFrame *frame;
	PurplePluginPref *pref;

	frame = purple_plugin_pref_frame_new();

	pref = purple_plugin_pref_new_with_label(_("Highlight"));
	purple_plugin_pref_frame_add(frame, pref);

	pref = purple_plugin_pref_new_with_name_and_label(PREF_HLWORDS,
					_("_Words to highlight on\n(separate the words with a blank space)"));
	purple_plugin_pref_frame_add(frame, pref);

	pref = purple_plugin_pref_new_with_label(_("Number of displayed characters"));
	purple_plugin_pref_frame_add(frame, pref);

	pref = purple_plugin_pref_new_with_name_and_label(PREF_CHARS,
					_("_Set the number of characters displayed\nin the nicksaid menu"));
	purple_plugin_pref_set_bounds(pref, 10, 40);
	purple_plugin_pref_frame_add(frame, pref);

	pref = purple_plugin_pref_new_with_name_and_label(PREF_SHOWWHO,
					_("Display who said your name in the nicksaid menu"));
	purple_plugin_pref_frame_add(frame, pref);

	pref = purple_plugin_pref_new_with_name_and_label(PREF_TIMESTAMP,
					_("Display _timestamps in the nicksaid menu"));
	purple_plugin_pref_frame_add(frame, pref);

	pref = purple_plugin_pref_new_with_name_and_label(PREF_DATESTAMP,
					_("_Display _datestamps in the nicksaid menu"));
	purple_plugin_pref_frame_add(frame, pref);

	pref = purple_plugin_pref_new_with_name_and_label(PREF_SHOWALL,
					_("Allow displaying in a separate dialog"));
	purple_plugin_pref_frame_add(frame, pref);

	return frame;
}

static PurplePluginUiInfo prefs_info = {
	get_plugin_pref_frame, 0, NULL
};

static PurplePluginInfo info =
{
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
	PP_VERSION,				/* version				*/
	NULL,						/* summary				*/
	NULL,						/* description			*/
	PLUGIN_AUTHOR,				/* author				*/
	PP_WEBSITE,				/* website				*/

	plugin_load,				/* load					*/
	plugin_unload,				/* unload				*/
	NULL,						/* destroy				*/

	NULL,						/* ui_info				*/
	NULL,						/* extra_info			*/
	&prefs_info,				/* prefs_info			*/
	NULL						/* actions				*/
};

static void
init_plugin(PurplePlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _(PLUGIN_NAME);
	info.summary = _(PLUGIN_SUMMARY);
	info.description = _(PLUGIN_DESCRIPTION);

	purple_prefs_add_none(PREF_PREFIX);
	purple_prefs_add_string(PREF_HLWORDS, "");
	purple_prefs_add_int(PREF_CHARS, 15);
	purple_prefs_add_bool(PREF_TIMESTAMP, TRUE);
	purple_prefs_add_bool(PREF_DATESTAMP, FALSE);
	purple_prefs_add_bool(PREF_SHOWWHO, TRUE);
	purple_prefs_add_bool(PREF_SHOWALL, FALSE);
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info)

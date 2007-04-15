/*
 * Purple-Schedule - Schedule reminders at specified times.
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

#define PLUGIN_ID			"gtk-gaim-schedule"
#define PLUGIN_NAME			"Purple Schedule"
#define PLUGIN_STATIC_NAME	"Purple Schedule"
#define PLUGIN_SUMMARY		"Schedule reminders at specified times."
#define PLUGIN_DESCRIPTION	"Schedule reminders at specified times."
#define PLUGIN_AUTHOR		"Sadrul H Chowdhury <sadrul@users.sourceforge.net>"

/* System headers */
#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>

/* Purple headers */
#include <gtkplugin.h>
#include <version.h>

#include <gtkimhtml.h>
#include <gtkutils.h>

#include "schedule.h"

#define	NEXT_ROW	row++; col = 0
#define	NEXT_COL	col++

typedef struct _ScheduleWindow ScheduleWindow;

struct _ScheduleWindow
{
	GtkWidget *window;

	GtkWidget *treeview;
	GtkListStore *model;

	GtkWidget *right_container;

	GtkWidget *name;

	GtkWidget *radio_day;
	GtkWidget *radio_date;
	GtkWidget *month;
	GtkWidget *year;
	GtkWidget *day;
	GtkWidget *date;
	GtkWidget *hour;
	GtkWidget *minute;

	GtkWidget *eyear;
	GtkWidget *eday;

	GtkWidget *check_send;
	GtkWidget *check_popup;
	GtkWidget *check_status;

	GtkWidget *accounts;
	GtkWidget *buddy;
	GtkWidget *imhtml;

	GtkWidget *popup_message;

	GtkWidget *statuslist;
};

static ScheduleWindow *win;

static void
add_columns(ScheduleWindow *win)
{
	GtkTreeViewColumn *col;
	GtkCellRenderer *rend;

	rend = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Schedule List"), rend,
							"text", 0, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(win->treeview), col);
}

static void
populate_list(ScheduleWindow *win)
{
	GList *list = purple_schedules_get_all();

	gtk_list_store_clear(win->model);
	while (list)
	{
		GtkTreeIter iter;
		PurpleSchedule *schedule = list->data;

		gtk_list_store_append(win->model, &iter);
		gtk_list_store_set(win->model, &iter, 0, schedule->name, 1, schedule, -1);

		list = list->next;
	}
}

static void
schedule_window_destroy()
{
	if (!win)
		return;

	gtk_widget_destroy(win->window);

	g_free(win);

	win = NULL;
}

static GtkWidget *
gtk_left_label_new(const char *text)
{
	GtkWidget *label = gtk_label_new_with_mnemonic(text);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	return label;
}

static void
disable_widget(GtkWidget *wid, GtkWidget *target)
{
	gtk_widget_set_sensitive(target, !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid)));
}

static void
add_date_time_fields(ScheduleWindow *win, GtkWidget *box)
{
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *day, *date;
	GtkWidget *combo, *spin, *sbox, *label;
	const char *months[] = { _("Every month"),
	                         _("January"),
	                         _("February"),
	                         _("March"),
	                         _("April"),
	                         _("May"),
	                         _("June"),
	                         _("July"),
	                         _("August"),
	                         _("September"),
	                         _("October"),
	                         _("November"),
	                         _("December"),
	                         NULL
	};
	const char *days[] =  { _("Everyday"),
	                        _("Sunday"),
	                        _("Monday"),
	                        _("Tuesday"),
	                        _("Wednesday"),
	                        _("Thursday"),
	                        _("Friday"),
	                        _("Saturday"),
	                        NULL
	};
	int i;
	int row = 0, col = 0;
	time_t now = time(NULL);
	struct tm *tm = localtime(&now);

	frame = pidgin_make_frame(box, _("Select Date and Time"));

	table = gtk_table_new(4, 5, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), PIDGIN_HIG_BOX_SPACE);
	gtk_table_set_col_spacings(GTK_TABLE(table), PIDGIN_HIG_BOX_SPACE);
	gtk_container_add(GTK_CONTAINER(frame), table);

#define ATTACH(wid) \
	do {\
		gtk_table_attach(GTK_TABLE(table), wid, \
								col, col+1, row, row+1, \
								0, 0, 0, 0); \
		col++; \
	} while (0)

	ATTACH(label = gtk_left_label_new(_("Month")));
	win->month = combo = gtk_combo_box_new_text();
	for (i = 0; months[i]; i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), months[i]);
	}
	ATTACH(combo);

	ATTACH(label = gtk_left_label_new(_("Year")));
	win->year = spin = gtk_spin_button_new_with_range(1900. + tm->tm_year, 9999.0, 1.0);
	ATTACH(spin);
	ATTACH(win->eyear = gtk_check_button_new_with_mnemonic(_("Every Year")));

	NEXT_ROW;

	win->radio_day = day = gtk_radio_button_new_with_mnemonic(NULL, _("Day"));
	ATTACH(day);
	win->day = combo = gtk_combo_box_new_text();
	for (i = 0; days[i]; i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), days[i]);
	}
	ATTACH(combo);

	win->radio_date = date = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(day), _("Date"));
	ATTACH(date);
	win->date = spin = gtk_spin_button_new_with_range(0., 31.0, 1.0);
	ATTACH(spin);
	ATTACH(win->eday = gtk_check_button_new_with_mnemonic(_("Everyday")));

	NEXT_ROW;

	ATTACH(label = gtk_left_label_new(_("Time")));

	sbox = gtk_hbox_new(FALSE, 0);
	/* Spin for hour */
	win->hour = spin = gtk_spin_button_new_with_range(-1., 23.0, 1.0);
	gtk_box_pack_start(GTK_BOX(sbox), spin, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(sbox), gtk_label_new(" : "), FALSE, FALSE, 2);
	win->minute = spin = gtk_spin_button_new_with_range(-1., 59.0, 1.0);
	gtk_box_pack_start(GTK_BOX(sbox), spin, FALSE, FALSE, 0);
	
	ATTACH(sbox);

	g_signal_connect(G_OBJECT(win->eyear), "toggled", G_CALLBACK(disable_widget),
					win->year);
	g_signal_connect(G_OBJECT(win->eday), "toggled", G_CALLBACK(disable_widget),
					win->date);

	/* Disable this as long as it's not implemented. */
	gtk_widget_set_sensitive(win->radio_day, FALSE);
	gtk_widget_set_sensitive(win->day, FALSE);
}

static void
toggle_send_message_cb(GtkWidget *w, GtkWidget *table)
{
	gtk_widget_set_sensitive(table, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
}

static void
add_send_message_fields(ScheduleWindow *win, GtkWidget *box)
{
	GtkWidget *frame, *table, *enable, *check;
	GtkWidget *optmenu, *entry, *imhtml, *fr;

	frame = pidgin_make_frame(box, _("Send Message"));

	enable = gtk_vbox_new(FALSE, PIDGIN_HIG_BOX_SPACE);
	gtk_container_add(GTK_CONTAINER(frame), enable);

	table = gtk_table_new(4, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), PIDGIN_HIG_BOX_SPACE);
	gtk_table_set_col_spacings(GTK_TABLE(table), PIDGIN_HIG_BOX_SPACE);
	gtk_widget_set_sensitive(table, FALSE);

	win->check_send = check = gtk_check_button_new_with_mnemonic(_("_Send message to a friend"));
	g_signal_connect(G_OBJECT(check), "clicked", G_CALLBACK(toggle_send_message_cb), table);

	gtk_box_pack_start(GTK_BOX(enable), check, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(enable), table, TRUE, TRUE, 0);

	win->accounts = optmenu = pidgin_account_option_menu_new(NULL, TRUE, NULL, NULL, NULL);
	win->buddy = entry = gtk_entry_new();
	pidgin_setup_screenname_autocomplete(entry, optmenu, FALSE);
	fr = pidgin_create_imhtml(TRUE, &imhtml, NULL, NULL);
	win->imhtml = imhtml;
	/* XXX: set the formatting to default send-message format */

	gtk_table_attach(GTK_TABLE(table), gtk_left_label_new(_("Buddy")),
							0, 1, 0, 1,
							GTK_FILL, 0, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), entry, 1, 2, 0, 1);

	gtk_table_attach(GTK_TABLE(table), gtk_left_label_new(_("Account")),
							0, 1, 1, 2, GTK_FILL, 0, 0, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), optmenu, 1, 2, 1, 2);

	gtk_table_attach_defaults(GTK_TABLE(table), gtk_left_label_new(_("Message")),
							0, 2, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(table), fr, 0, 2, 3, 4);
}

static void
add_popup_fields(ScheduleWindow *win, GtkWidget *box)
{
	GtkWidget *frame, *enable, *check;
	GtkWidget *entry;

	frame = pidgin_make_frame(box, _("Popup Dialog"));

	enable = gtk_hbox_new(FALSE, PIDGIN_HIG_BOX_SPACE);
	gtk_container_add(GTK_CONTAINER(frame), enable);

	win->check_popup = check = gtk_check_button_new_with_mnemonic(_("_Popup a reminder dialog with message"));
	win->popup_message = entry = gtk_entry_new();
	
	gtk_widget_set_sensitive(entry, FALSE);
	g_signal_connect(G_OBJECT(check), "clicked", G_CALLBACK(toggle_send_message_cb), entry);

	gtk_box_pack_start(GTK_BOX(enable), check, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(enable), entry, TRUE, TRUE, 0);
}

static void
schedule_selection_changed_cb(GtkTreeSelection *sel, ScheduleWindow *win)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	PurpleSchedule *schedule;
	GList *list;

	if (!gtk_tree_selection_get_selected(sel, &model, &iter))
	{
		gtk_widget_set_sensitive(win->right_container, FALSE);
		return;
	}

	gtk_widget_set_sensitive(win->right_container, TRUE);

	gtk_tree_model_get(model, &iter, 1, &schedule, -1);

	gtk_entry_set_text(GTK_ENTRY(win->name), schedule->name);

	if (schedule->type == PURPLE_SCHEDULE_TYPE_DATE)
	{
		if (schedule->d.date == -1)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->eday), TRUE);
		else
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->eday), FALSE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->radio_date), TRUE);

		gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->date), (double)schedule->d.date + 1);
		gtk_combo_box_set_active(GTK_COMBO_BOX(win->day), -1);
	}
	else
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->radio_day), TRUE);
		
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->date), -1.0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(win->day), schedule->d.day + 1);
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(win->month), schedule->month + 1);
	if (schedule->year == -1)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->eyear), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->eyear), FALSE);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->year), schedule->year);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->hour), schedule->hour);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->minute), schedule->minute);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->check_send), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->check_popup), FALSE);
	/*gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->check_status), FALSE);*/

	for (list = schedule->actions; list; list = list->next)
	{
		ScheduleAction *action = list->data;

		switch (action->type)
		{
			case SCHEDULE_ACTION_POPUP:
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->check_popup), TRUE);
				gtk_entry_set_text(GTK_ENTRY(win->popup_message), action->d.popup_message);
				break;
			case SCHEDULE_ACTION_CONV:
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->check_send), TRUE);
				pidgin_account_option_menu_set_selected(win->accounts, action->d.send.account);
				gtk_entry_set_text(GTK_ENTRY(win->buddy), action->d.send.who);
				gtk_imhtml_clear(GTK_IMHTML(win->imhtml));
				/* XXX: gtk_imhtml_set_protocol_name*/
				gtk_imhtml_append_text(GTK_IMHTML(win->imhtml), action->d.send.message, 0);
				break;
			default:
				purple_debug_warning("gaim-schedule", "action type not implemented yet.\n");
				break;
		}
	}
}

static void
add_name_field(ScheduleWindow *win, GtkWidget *box)
{
	GtkWidget *label, *pack, *entry;

	pack = gtk_hbox_new(FALSE, PIDGIN_HIG_BOX_SPACE);

	label = gtk_label_new(_("Name"));
	gtk_box_pack_start(GTK_BOX(pack), label, FALSE, FALSE, 0);

	win->name = entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(pack), entry, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(box), pack, FALSE, TRUE, 0);
}

static void
save_clicked_cb(GtkWidget *w, ScheduleWindow *win)
{
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	PurpleSchedule *schedule;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(win->treeview));
	gtk_tree_selection_get_selected(sel, NULL, &iter);

	gtk_tree_model_get(GTK_TREE_MODEL(win->model), &iter, 1, &schedule, -1);

	g_free(schedule->name);
	schedule->name = g_strdup(gtk_entry_get_text(GTK_ENTRY(win->name)));

	gtk_list_store_set(win->model, &iter, 0, schedule->name, -1);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->radio_day))) {
		schedule->type = PURPLE_SCHEDULE_TYPE_DAY;
		schedule->d.day = gtk_combo_box_get_active(GTK_COMBO_BOX(win->day)) - 1;
	} else {
		schedule->type = PURPLE_SCHEDULE_TYPE_DATE;
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->eday)))
			schedule->d.date = -1;
		else
			schedule->d.date = gtk_spin_button_get_value(GTK_SPIN_BUTTON(win->date)) - 1;
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->eyear)))
		schedule->year = -1;
	else
		schedule->year = gtk_spin_button_get_value(GTK_SPIN_BUTTON(win->year));

	schedule->month = gtk_combo_box_get_active(GTK_COMBO_BOX(win->month)) - 1;
	schedule->hour = gtk_spin_button_get_value(GTK_SPIN_BUTTON(win->hour));
	schedule->minute = gtk_spin_button_get_value(GTK_SPIN_BUTTON(win->minute));

	purple_schedule_remove_action(schedule, SCHEDULE_ACTION_POPUP);
	purple_schedule_remove_action(schedule, SCHEDULE_ACTION_CONV);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->check_send))) {
		char *message = gtk_imhtml_get_markup(GTK_IMHTML(win->imhtml));
		purple_schedule_add_action(schedule, SCHEDULE_ACTION_CONV,
						message, gtk_entry_get_text(GTK_ENTRY(win->buddy)),
						pidgin_account_option_menu_get_selected(win->accounts));
		g_free(message);
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->check_popup))) {
		purple_schedule_add_action(schedule, SCHEDULE_ACTION_POPUP,
					gtk_entry_get_text(GTK_ENTRY(win->popup_message)));
	}

	purple_schedule_reschedule(schedule);
	if (!g_list_find(purple_schedules_get_all(), schedule))
		purple_schedules_add(schedule);
	
	purple_schedules_sync();
}

static gboolean
find_schedule_by_name(const char *name, ScheduleWindow *win)
{
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(win->model), &iter))
		return FALSE;

	do
	{
		char *text;
		gtk_tree_model_get(GTK_TREE_MODEL(win->model), &iter, 0, &text, -1);
		if (g_utf8_collate(name, text) == 0)
		{
			g_free(text);
			return TRUE;
		}
		g_free(text);
	} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(win->model), &iter));

	return FALSE;
}

static void
add_schedule_cb(GtkWidget *b, ScheduleWindow *win)
{
	PurpleSchedule *schedule;
	char *name;
	int count = 1;
	GtkTreeIter iter;
	GtkTreePath *path;

	schedule = purple_schedule_new();
	name = g_strdup("Schedule");
	while (find_schedule_by_name(name, win))
	{
		g_free(name);
		name = g_strdup_printf("Schedule<%d>", count++);
	}
	schedule->name = name;

	gtk_list_store_append(win->model, &iter);
	gtk_list_store_set(win->model, &iter, 0, schedule->name, 1, schedule, -1);

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(win->model), &iter);
	gtk_tree_selection_select_path(gtk_tree_view_get_selection(GTK_TREE_VIEW(win->treeview)), path);
	gtk_tree_path_free(path);
	
	return;
}

static void
delete_schedule_cb(GtkWidget *b, ScheduleWindow *win)
{
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;
	PurpleSchedule *schedule;

	/* XXX: ask for confirmation? */

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(win->treeview));
	
	if (!gtk_tree_selection_get_selected(sel, &model, &iter))
		return;

	gtk_tree_model_get(model, &iter, 1, &schedule, -1);
	gtk_list_store_remove(win->model, &iter);
	purple_schedule_destroy(schedule);
}

static void
schedule_window_show(gboolean new)
{
	GtkWidget *box, *box2, *bbox;
	GtkTreeSelection *sel;
	GtkWidget *sw, *button;

	if (win != NULL)
	{
		gtk_window_present(GTK_WINDOW(win->window));
		return;
	}

	win = g_new0(ScheduleWindow, 1);

	win->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_resizable(GTK_WINDOW(win->window), TRUE);
	g_signal_connect(G_OBJECT(win->window), "delete_event", schedule_window_destroy, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(win->window), PIDGIN_HIG_BOX_SPACE);

	box = gtk_hbox_new(FALSE, PIDGIN_HIG_BOX_SPACE);
	gtk_container_add(GTK_CONTAINER(win->window), box);

	win->model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);

	win->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(win->model));
	add_columns(win);
	populate_list(win);
	
	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(sw), win->treeview);

	box2 = gtk_vbox_new(FALSE, PIDGIN_HIG_BOX_SPACE);
	gtk_box_pack_start(GTK_BOX(box2), sw, TRUE, TRUE, 0);

	bbox = gtk_hbutton_box_new();
	button = pidgin_pixbuf_button_from_stock(_("_Add"), GTK_STOCK_ADD, PIDGIN_BUTTON_HORIZONTAL);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(add_schedule_cb), win);
	gtk_box_pack_start(GTK_BOX(bbox), button, FALSE, TRUE, 0);

	button = pidgin_pixbuf_button_from_stock(_("_Delete"), GTK_STOCK_CANCEL, PIDGIN_BUTTON_HORIZONTAL);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(delete_schedule_cb), win);
	gtk_box_pack_start(GTK_BOX(bbox), button, FALSE, TRUE, 0);
	
	gtk_box_pack_start(GTK_BOX(box2), bbox, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(box), box2, FALSE, FALSE, PIDGIN_HIG_BOX_SPACE);

	win->right_container = box2 = gtk_vbox_new(FALSE, PIDGIN_HIG_BOX_SPACE);
	gtk_box_pack_start(GTK_BOX(box), box2, TRUE, TRUE, PIDGIN_HIG_BOX_SPACE);

	add_name_field(win, box2);
	add_date_time_fields(win, box2);
	add_send_message_fields(win, box2);
	add_popup_fields(win, box2);
	/* TODO: Option to change status to a savedstatus */
	/*add_change_status_fields(win, box2);*/

	gtk_widget_set_sensitive(win->right_container, FALSE);

	box = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(box), GTK_BUTTONBOX_END);

	button = gtk_button_new_from_stock(GTK_STOCK_SAVE);
	gtk_box_pack_end(GTK_BOX(box), button, FALSE, TRUE, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(save_clicked_cb), win);

	gtk_box_pack_start(GTK_BOX(box2), box, FALSE, FALSE, 0);

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(win->treeview));
	g_signal_connect(G_OBJECT(sel), "changed", G_CALLBACK(schedule_selection_changed_cb), win);

	if (new)
	{
		add_schedule_cb(NULL, win);
	}
	else
	{
		GtkTreePath *path = gtk_tree_path_new_first();
		gtk_tree_selection_select_path(sel, path);
		gtk_tree_path_free(path);
	}

	gtk_widget_show_all(win->window);
}

static void
purple_schedule_window_new(PurplePluginAction *action)
{
	schedule_window_show(TRUE);
}

static void
purple_schedule_list(PurplePluginAction *action)
{
	schedule_window_show(FALSE);
}

static GList *
actions(PurplePlugin *plugin, gpointer context)
{
	GList *list = NULL;
	PurplePluginAction *act;

	/* XXX: submit the patch to Purple for making the mnemonics work */
	act = purple_plugin_action_new(_("New Schedule"), purple_schedule_window_new);
	list = g_list_append(list, act);

	act = purple_plugin_action_new(_("List of Schedules"), purple_schedule_list);
	list = g_list_append(list, act);

	return list;
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	purple_schedule_init();
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	schedule_window_destroy();
	purple_schedule_uninit();
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
	NULL,						/* prefs_info			*/
	actions						/* actions				*/
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
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info)

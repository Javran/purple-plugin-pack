/*
 * TimeLog plugin.
 *
 * Copyright (C) 2006 Jon Oberheide
 * Copyright (C) 2007-2008 Stu Tomlinson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

/* If you can't figure out what this line is for, DON'T TOUCH IT. */
#include "../common/pp_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "timelog.h"
#include "range-widget.h"

static GtkWidget *start_calendar;
static GtkWidget *end_calendar;
static GtkWidget *start_hours;
static GtkWidget *start_minutes;
static GtkWidget *start_seconds;
static GtkWidget *end_hours;
static GtkWidget *end_minutes;
static GtkWidget *end_seconds;

static void
calendar_update(GtkWidget *calendar, gint add)
{
	static const gint month_length[2][13] =
	{
		{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
		{ 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
	};

	gint day, month;
	guint year;
	gint days_in_month;
	gboolean leap_year;

	gtk_calendar_get_date(GTK_CALENDAR(calendar), &year, (guint*)&month, (guint*)&day);

	leap_year = g_date_is_leap_year(year);
	days_in_month = month_length[leap_year][month+1];

	if (add != 0) {
		day += add;
		if (day < 1) {
			day = (month_length[leap_year][month]) + day;
			month--;
		} else if (day > days_in_month) {
			day -= days_in_month;
			month++;
		}

		if (month < 0) {
			year--;
			leap_year = g_date_is_leap_year(year);
			month = 11;
			day = month_length[leap_year][month+1];
		} else if (month > 11) {
			year++;
			leap_year = g_date_is_leap_year(year);
			month = 0;
			day = 1;
		}
	}

	gtk_calendar_select_month(GTK_CALENDAR(calendar), month, year);
	gtk_calendar_select_day(GTK_CALENDAR(calendar), day);
}

static void
cb_time_value_changed(GtkSpinButton *widget, gpointer data)
{
	gchar *val;
	gint value = gtk_spin_button_get_value(widget);

	if (widget == GTK_SPIN_BUTTON(start_seconds)) {
		if (value > 59) {
			gtk_spin_button_set_value(widget, value - 60);
			gtk_spin_button_spin(GTK_SPIN_BUTTON(start_minutes), GTK_SPIN_STEP_FORWARD, 1);
		} else if (value < 0) {
			gtk_spin_button_set_value(widget, value + 60);
			gtk_spin_button_spin(GTK_SPIN_BUTTON(start_minutes), GTK_SPIN_STEP_BACKWARD, 1);
		}
	} else if (widget == GTK_SPIN_BUTTON(start_minutes)) {
		if (value > 59) {
			gtk_spin_button_set_value(widget, value - 60);
			gtk_spin_button_spin(GTK_SPIN_BUTTON(start_hours), GTK_SPIN_STEP_FORWARD, 1);
		} else if (value < 0) {
			gtk_spin_button_set_value(widget, value + 60);
			gtk_spin_button_spin(GTK_SPIN_BUTTON(start_hours), GTK_SPIN_STEP_BACKWARD, 1);
		}
	} else if (widget == GTK_SPIN_BUTTON(start_hours)) {
		if (value > 23) {
			gtk_spin_button_set_value(widget, value - 24);
			calendar_update(start_calendar, 1);
		} else if (value < 0) {
			calendar_update(start_calendar, -1);
			gtk_spin_button_set_value(widget, value + 24);
		}
	} else if (widget == GTK_SPIN_BUTTON(end_seconds)) {
		if (value > 59) {
			gtk_spin_button_set_value(widget, value - 60);
			gtk_spin_button_spin(GTK_SPIN_BUTTON(end_minutes), GTK_SPIN_STEP_FORWARD, 1);
		} else if (value < 0) {
			gtk_spin_button_set_value(widget, value + 60);
			gtk_spin_button_spin(GTK_SPIN_BUTTON(end_minutes), GTK_SPIN_STEP_BACKWARD, 1);
		}
	} else if (widget == GTK_SPIN_BUTTON(end_minutes)) {
		if (value > 59) {
			gtk_spin_button_set_value(widget, value - 60);
			gtk_spin_button_spin(GTK_SPIN_BUTTON(end_hours), GTK_SPIN_STEP_FORWARD, 1);
		} else if (value < 0) {
			gtk_spin_button_set_value(widget, value + 60);
			gtk_spin_button_spin(GTK_SPIN_BUTTON(end_hours), GTK_SPIN_STEP_BACKWARD, 1);
		}
	} else if (widget == GTK_SPIN_BUTTON(end_hours)) {
		if (value > 23) {
			gtk_spin_button_set_value(widget, value - 24);
			calendar_update(end_calendar, 1);
		} else if (value < 0) {
			calendar_update(end_calendar, -1);
			gtk_spin_button_set_value(widget, value + 24);
		}
	}

	val = g_strdup_printf("%02d", value);
	gtk_entry_set_text(GTK_ENTRY(widget), val);
	g_free(val);
}

void
range_widget_get_bounds(GtkWidget *dialog, time_t *start, time_t *end)
{
	guint year, month, day;
	struct tm start_tm, end_tm;

	memset(&start_tm, 0, sizeof(struct tm));
	memset(&end_tm, 0, sizeof(struct tm));

	gtk_calendar_get_date(GTK_CALENDAR(start_calendar), &year, &month, &day);
	start_tm.tm_year = year - 1900;
	start_tm.tm_mon = month;
	start_tm.tm_mday = day;
	start_tm.tm_hour = gtk_spin_button_get_value(GTK_SPIN_BUTTON(start_hours));
	start_tm.tm_min = gtk_spin_button_get_value(GTK_SPIN_BUTTON(start_minutes));
	start_tm.tm_sec = gtk_spin_button_get_value(GTK_SPIN_BUTTON(start_seconds));

	gtk_calendar_get_date(GTK_CALENDAR(end_calendar), &year, &month, &day);
	end_tm.tm_year = year - 1900;
	end_tm.tm_mon = month;
	end_tm.tm_mday = day;
	end_tm.tm_hour = gtk_spin_button_get_value(GTK_SPIN_BUTTON(end_hours));
	end_tm.tm_min = gtk_spin_button_get_value(GTK_SPIN_BUTTON(end_minutes));
	end_tm.tm_sec = gtk_spin_button_get_value(GTK_SPIN_BUTTON(end_seconds));

	*start = mktime(&start_tm);
	*end = mktime(&end_tm);
}

void
range_widget_destroy(GtkWidget *dialog)
{
	gtk_widget_destroy(dialog);
}

GtkWidget *
range_widget_create()
{
	GtkWidget *range_dialog;
	GtkWidget *dialog_vbox4;
	GtkWidget *vbox7;
	GtkWidget *hbox46;
	GtkWidget *label43;
	GtkWidget *label44;
	GtkWidget *hbox42;
	GtkWidget *hbox43;
	GtkWidget *hbox44;
	GtkObject *start_hours_adj;
	GtkObject *start_minutes_adj;
	GtkObject *start_seconds_adj;
	GtkWidget *hbox45;
	GtkObject *end_hours_adj;
	GtkObject *end_minutes_adj;
	GtkObject *end_seconds_adj;
	GtkWidget *dialog_action_area4;
	GtkWidget *cancel_button;
	GtkWidget *ok_button;
	GtkWidget *alignment55;
	GtkWidget *hbox341;
	GtkWidget *image49;
	GtkWidget *label97;
	gchar *val;

	range_dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(range_dialog), TIMELOG_TITLE);
	gtk_window_set_modal(GTK_WINDOW(range_dialog), FALSE);
	gtk_window_set_type_hint(GTK_WINDOW(range_dialog), GDK_WINDOW_TYPE_HINT_DIALOG);

	dialog_vbox4 = GTK_DIALOG(range_dialog)->vbox;
	gtk_widget_show(dialog_vbox4);

	vbox7 = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox7);
	gtk_box_pack_start(GTK_BOX(dialog_vbox4), vbox7, TRUE, TRUE, 0);

	hbox46 = gtk_hbox_new(FALSE, 20);
	gtk_widget_show(hbox46);
	gtk_box_pack_start(GTK_BOX(vbox7), hbox46, FALSE, FALSE, 5);

	label43 = gtk_label_new(_("Start Time"));
	gtk_widget_show(label43);
	gtk_box_pack_start(GTK_BOX(hbox46), label43, TRUE, TRUE, 0);

	label44 = gtk_label_new(_("End Time"));
	gtk_widget_show(label44);
	gtk_box_pack_start(GTK_BOX(hbox46), label44, TRUE, TRUE, 0);

	hbox42 = gtk_hbox_new(FALSE, 20);
	gtk_widget_show(hbox42);
	gtk_box_pack_start(GTK_BOX(vbox7), hbox42, TRUE, TRUE, 0);

	start_calendar = gtk_calendar_new();
	gtk_widget_show(start_calendar);
	gtk_box_pack_start(GTK_BOX(hbox42), start_calendar, TRUE, TRUE, 0);
	gtk_calendar_display_options(GTK_CALENDAR(start_calendar),
			GTK_CALENDAR_SHOW_HEADING | GTK_CALENDAR_SHOW_DAY_NAMES);

	end_calendar = gtk_calendar_new();
	gtk_widget_show(end_calendar);
	gtk_box_pack_start(GTK_BOX(hbox42), end_calendar, TRUE, TRUE, 0);
	gtk_calendar_display_options(GTK_CALENDAR(end_calendar),
			GTK_CALENDAR_SHOW_HEADING | GTK_CALENDAR_SHOW_DAY_NAMES);

	hbox43 = gtk_hbox_new(FALSE, 20);
	gtk_widget_show(hbox43);
	gtk_box_pack_start(GTK_BOX(vbox7), hbox43, FALSE, TRUE, 10);

	hbox44 = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox44);
	gtk_box_pack_start(GTK_BOX(hbox43), hbox44, TRUE, TRUE, 0);

	start_hours_adj = gtk_adjustment_new(0, -100, 100, 1, 10, 10);
	start_hours = gtk_spin_button_new(GTK_ADJUSTMENT(start_hours_adj), 1, 0);
	gtk_widget_show(start_hours);
	gtk_box_pack_start(GTK_BOX(hbox44), start_hours, TRUE, TRUE, 0);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(start_hours), TRUE);
	gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(start_hours), TRUE);

	start_minutes_adj = gtk_adjustment_new(0, -100, 100, 1, 10, 10);
	start_minutes = gtk_spin_button_new(GTK_ADJUSTMENT(start_minutes_adj), 1, 0);
	gtk_widget_show(start_minutes);
	gtk_box_pack_start(GTK_BOX(hbox44), start_minutes, TRUE, TRUE, 0);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(start_minutes), TRUE);
	gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(start_minutes), TRUE);

	start_seconds_adj = gtk_adjustment_new(0, -100, 100, 1, 10, 10);
	start_seconds = gtk_spin_button_new(GTK_ADJUSTMENT(start_seconds_adj), 1, 0);
	gtk_widget_show(start_seconds);
	gtk_box_pack_start(GTK_BOX(hbox44), start_seconds, TRUE, TRUE, 0);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(start_seconds), TRUE);
	gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(start_seconds), TRUE);

	hbox45 = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox45);
	gtk_box_pack_start(GTK_BOX(hbox43), hbox45, TRUE, TRUE, 0);

	end_hours_adj = gtk_adjustment_new(0, -100, 100, 1, 10, 10);
	end_hours = gtk_spin_button_new(GTK_ADJUSTMENT(end_hours_adj), 1, 0);
	gtk_widget_show(end_hours);
	gtk_box_pack_start(GTK_BOX(hbox45), end_hours, TRUE, TRUE, 0);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(end_hours), TRUE);
	gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(end_hours), TRUE);

	end_minutes_adj = gtk_adjustment_new(0, -100, 100, 1, 10, 10);
	end_minutes = gtk_spin_button_new(GTK_ADJUSTMENT(end_minutes_adj), 1, 0);
	gtk_widget_show(end_minutes);
	gtk_box_pack_start(GTK_BOX(hbox45), end_minutes, TRUE, TRUE, 0);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(end_minutes), TRUE);
	gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(end_minutes), TRUE);

	end_seconds_adj = gtk_adjustment_new(0, -100, 100, 1, 10, 10);
	end_seconds = gtk_spin_button_new(GTK_ADJUSTMENT(end_seconds_adj), 1, 0);
	gtk_widget_show(end_seconds);
	gtk_box_pack_start(GTK_BOX(hbox45), end_seconds, TRUE, TRUE, 0);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(end_seconds), TRUE);
	gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(end_seconds), TRUE);

	dialog_action_area4 = GTK_DIALOG(range_dialog)->action_area;
	gtk_widget_show(dialog_action_area4);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area4), GTK_BUTTONBOX_END);

	cancel_button = gtk_button_new_from_stock("gtk-cancel");
	gtk_widget_show(cancel_button);
	gtk_dialog_add_action_widget(GTK_DIALOG(range_dialog), cancel_button, GTK_RESPONSE_CANCEL);
	GTK_WIDGET_SET_FLAGS(cancel_button, GTK_CAN_DEFAULT);

	ok_button = gtk_button_new();
	gtk_widget_show(ok_button);
	gtk_dialog_add_action_widget(GTK_DIALOG(range_dialog), ok_button, GTK_RESPONSE_OK);
	GTK_WIDGET_SET_FLAGS(ok_button, GTK_CAN_DEFAULT);

	alignment55 = gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_widget_show(alignment55);
	gtk_container_add(GTK_CONTAINER(ok_button), alignment55);

	hbox341 = gtk_hbox_new(FALSE, 2);
	gtk_widget_show(hbox341);
	gtk_container_add(GTK_CONTAINER(alignment55), hbox341);

	image49 = gtk_image_new_from_stock("gtk-ok", GTK_ICON_SIZE_BUTTON);
	gtk_widget_show(image49);
	gtk_box_pack_start(GTK_BOX(hbox341), image49, FALSE, FALSE, 0);

	label97 = gtk_label_new_with_mnemonic(_("Select Time Range"));
	gtk_widget_show(label97);
	gtk_box_pack_start(GTK_BOX(hbox341), label97, FALSE, FALSE, 0);

	g_signal_connect((gpointer) start_hours, "value_changed", 
			G_CALLBACK(cb_time_value_changed), NULL);
	g_signal_connect((gpointer) start_minutes, "value_changed", 
			G_CALLBACK(cb_time_value_changed), NULL);
	g_signal_connect((gpointer) start_seconds, "value_changed",
			G_CALLBACK(cb_time_value_changed), NULL);
	g_signal_connect((gpointer) end_hours, "value_changed",
			G_CALLBACK(cb_time_value_changed), NULL);
	g_signal_connect((gpointer) end_minutes, "value_changed",
			G_CALLBACK(cb_time_value_changed), NULL);
	g_signal_connect((gpointer) end_seconds, "value_changed",
			G_CALLBACK(cb_time_value_changed), NULL);

	val = g_strdup_printf("%02d", 0);
	gtk_entry_set_text(GTK_ENTRY(start_hours), val);
	gtk_entry_set_text(GTK_ENTRY(start_minutes), val);
	gtk_entry_set_text(GTK_ENTRY(start_seconds), val);
	gtk_entry_set_text(GTK_ENTRY(end_hours), val);
	gtk_entry_set_text(GTK_ENTRY(end_minutes), val);
	gtk_entry_set_text(GTK_ENTRY(end_seconds), val);
	g_free(val);

	gtk_widget_grab_default(ok_button);

	return range_dialog;
}

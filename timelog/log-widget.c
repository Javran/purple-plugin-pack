/*
 * TimeLog plugin.
 *
 * Copyright (c) 2006 Jon Oberheide.
 *
 * Based on code from Gaim's gtklog.c
 *
 * $Id: log-widget.c,v 1.30 2005/12/24 05:14:55 binaryjono Exp $
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib.h>
#include <gtk/gtk.h>

#include <pidgin.h>
#include <account.h>
#include <gtkblist.h>
#include <gtkutils.h>
#include <gtkimhtml.h>
#include <gtklog.h>
#include <log.h>
#include <notify.h>
#include <util.h>

#include "timelog.h"
#include "log-widget.h"

static PidginLogViewer *syslog_viewer = NULL;

static gint
log_compare(gconstpointer y, gconstpointer z)
{
	const PurpleLog *a = y;
	const PurpleLog *b = z;

	return strcmp(a->name, b->name);
}

static void
populate_log_tree(PidginLogViewer *lv)
{
	char name[64];
	char title[64];
	char prev_top_name[64] = "";
	char *utf8_tmp; /* temporary variable for utf8 conversion */
	GtkTreeIter toplevel, child;
	GList *logs = lv->logs;

	while (logs != NULL) {
		PurpleLog *log = logs->data;

		strncpy(name, log->name, sizeof(name));
		strftime(title, sizeof(title), "%c", localtime(&log->time));

		/* do utf8 conversions */
		utf8_tmp = purple_utf8_try_convert(name);
		strncpy(name, utf8_tmp, sizeof(name));
		g_free(utf8_tmp);
		utf8_tmp = purple_utf8_try_convert(title);
		strncpy(title, utf8_tmp, sizeof(title));
		g_free(utf8_tmp);

		if (strncmp(name, prev_top_name, sizeof(name)) != 0) {
			/* top level */
			gtk_tree_store_append(lv->treestore, &toplevel, NULL);
			gtk_tree_store_set(lv->treestore, &toplevel, 0, name, 1, NULL, -1);

			strncpy(prev_top_name, name, sizeof(prev_top_name));
		}

		/* sub */
		gtk_tree_store_append(lv->treestore, &child, &toplevel);
		gtk_tree_store_set(lv->treestore, &child, 0, title, 1, log, -1);

		logs = logs->next;
	}
}

static void
search_cb(GtkWidget *button, PidginLogViewer *lv)
{
	const char *search_term = gtk_entry_get_text(GTK_ENTRY(lv->entry));
	GList *logs;
	GdkCursor *cursor;

	if (lv->search != NULL)
		g_free(lv->search);

	gtk_tree_store_clear(lv->treestore);
	if (!(*search_term)) {
		/* reset the tree */
		populate_log_tree(lv);
		lv->search = NULL;
		gtk_imhtml_search_clear(GTK_IMHTML(lv->imhtml));
		return;
	}

	lv->search = g_strdup(search_term);

	cursor = gdk_cursor_new(GDK_WATCH);
	gdk_window_set_cursor(lv->window->window, cursor);
	gdk_cursor_unref(cursor);
	while (gtk_events_pending())
		gtk_main_iteration();

	for (logs = lv->logs; logs != NULL; logs = logs->next) {
		char *read = purple_log_read((PurpleLog*)logs->data, NULL);
		if (read && *read && purple_strcasestr(read, search_term)) {
			GtkTreeIter iter;
			PurpleLog *log = logs->data;
			char title[64];
			char *title_utf8; /* temporary variable for utf8 conversion */

			strftime(title, sizeof(title), "%c", localtime(&log->time));
			title_utf8 = purple_utf8_try_convert(title);
			strncpy(title, title_utf8, sizeof(title));
			g_free(title_utf8);

			gtk_tree_store_append (lv->treestore, &iter, NULL);
			gtk_tree_store_set(lv->treestore, &iter,
					   0, title,
					   1, log, -1);
		}
		g_free(read);
	}

	gdk_window_set_cursor(lv->window->window, NULL);
}

static gboolean
destroy_cb(GtkWidget *w, gint resp, gpointer data)
{
	PidginLogViewer *lv = syslog_viewer;
	syslog_viewer = NULL;

	while (lv->logs != NULL) {
		GList *logs2;

		purple_log_free((PurpleLog *)lv->logs->data);

		logs2 = lv->logs->next;
		g_list_free_1(lv->logs);
		lv->logs = logs2;
	}

	if (lv->search != NULL)
		g_free(lv->search);

	g_free(lv);
	gtk_widget_destroy(w);

	return TRUE;
}

static void
log_row_activated_cb(GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *col, PidginLogViewer *viewer)
{
	if (gtk_tree_view_row_expanded(tv, path)) {
		gtk_tree_view_collapse_row(tv, path);
	} else {
		gtk_tree_view_expand_row(tv, path, FALSE);
	}
}

static void
log_select_cb(GtkTreeSelection *sel, PidginLogViewer *viewer)
{
	GtkTreeIter iter;
	GValue val;
	GtkTreeModel *model = GTK_TREE_MODEL(viewer->treestore);
	PurpleLog *log = NULL;
	GdkCursor *cursor;
	PurpleLogReadFlags flags;
	char *read = NULL;
	char time[64];

	if (!gtk_tree_selection_get_selected(sel, &model, &iter)) {
		return;
	}

	val.g_type = 0;
	gtk_tree_model_get_value (model, &iter, 1, &val);
	log = g_value_get_pointer(&val);
	g_value_unset(&val);

	if (log == NULL) {
		return;
	}

	/* When we set the initial log, this gets called and the window is still NULL. */
	if (viewer->window->window != NULL) {
		cursor = gdk_cursor_new(GDK_WATCH);
		gdk_window_set_cursor(viewer->window->window, cursor);
		gdk_cursor_unref(cursor);
		while (gtk_events_pending())
			gtk_main_iteration();
	}

	if (log->type != PURPLE_LOG_SYSTEM) {
		char *title;
		char *title_utf8; /* temporary variable for utf8 conversion */

		strftime(time, sizeof(time), "%c", localtime(&log->time));

		if (log->type == PURPLE_LOG_CHAT)
			title = g_strdup_printf(_("Conversation in %s on %s"), log->name, time);
		else
			title = g_strdup_printf(_("Conversation with %s on %s"), log->name, time);

		title_utf8 = purple_utf8_try_convert(title);
		g_free(title);

		title = g_strdup_printf("<span size='larger' weight='bold'>%s</span>", title_utf8);
		g_free(title_utf8);

		gtk_label_set_markup(GTK_LABEL(viewer->label), title);
		g_free(title);
	}

	read = purple_log_read(log, &flags);
	viewer->flags = flags;

	gtk_imhtml_clear(GTK_IMHTML(viewer->imhtml));
	gtk_imhtml_set_protocol_name(GTK_IMHTML(viewer->imhtml),
				    purple_account_get_protocol_name(log->account));
	gtk_imhtml_append_text(GTK_IMHTML(viewer->imhtml), read,
			       GTK_IMHTML_NO_COMMENTS | GTK_IMHTML_NO_TITLE | GTK_IMHTML_NO_SCROLL |
			       ((flags & PURPLE_LOG_READ_NO_NEWLINE) ? GTK_IMHTML_NO_NEWLINE : 0));
	g_free(read);

	if (viewer->search != NULL) {
		gtk_imhtml_search_clear(GTK_IMHTML(viewer->imhtml));
		gtk_imhtml_search_find(GTK_IMHTML(viewer->imhtml), viewer->search);
	}

	/* When we set the initial log, this gets called and the window is still NULL. */
	if (viewer->window->window != NULL)
		gdk_window_set_cursor(viewer->window->window, NULL);
}

void
log_widget_display_logs(GList *logs)
{
	PidginLogViewer *lv;
	GtkWidget *title_box;
	char *text;
	GtkWidget *pane;
	GtkWidget *sw;
	GtkCellRenderer *rend;
	GtkTreeViewColumn *col;
	GtkTreeSelection *sel;
#if GTK_CHECK_VERSION(2,2,0)
	GtkTreePath *path_to_first_log;
#endif
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *button;

	if (syslog_viewer != NULL) {
		gtk_window_present(GTK_WINDOW(syslog_viewer->window));
		return;
	}

	lv = g_new0(PidginLogViewer, 1);
	lv->logs = logs;

	if (logs == NULL) {
		/* No logs were found. */
		purple_notify_info(NULL, TIMELOG_TITLE, _("No logs were found"), NULL);
		return;
	}

	lv->logs = g_list_sort(lv->logs, log_compare);

	/* Window ***********/
	lv->window = gtk_dialog_new_with_buttons(TIMELOG_TITLE, NULL, 0,
					     GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
	gtk_container_set_border_width (GTK_CONTAINER(lv->window), PIDGIN_HIG_BOX_SPACE);
	gtk_dialog_set_has_separator(GTK_DIALOG(lv->window), FALSE);
	gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(lv->window)->vbox), 0);
	g_signal_connect(G_OBJECT(lv->window), "response", G_CALLBACK(destroy_cb), NULL);
	gtk_window_set_role(GTK_WINDOW(lv->window), "log_viewer");

	title_box = GTK_DIALOG(lv->window)->vbox;

	/* Label ************/
	lv->label = gtk_label_new(NULL);

	text = g_strdup_printf("<span size='larger' weight='bold'>%s</span>", TIMELOG_TITLE);

	gtk_label_set_markup(GTK_LABEL(lv->label), text);
	gtk_misc_set_alignment(GTK_MISC(lv->label), 0, 0);
	gtk_box_pack_start(GTK_BOX(title_box), lv->label, FALSE, FALSE, 0);
	g_free(text);

	/* Pane *************/
	pane = gtk_hpaned_new();
	gtk_container_set_border_width(GTK_CONTAINER(pane), PIDGIN_HIG_BOX_SPACE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(lv->window)->vbox), pane, TRUE, TRUE, 0);

	/* List *************/
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_paned_add1(GTK_PANED(pane), sw);
	lv->treestore = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
	lv->treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (lv->treestore));
	rend = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes ("time", rend, "markup", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(lv->treeview), col);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (lv->treeview), FALSE);
	gtk_container_add (GTK_CONTAINER (sw), lv->treeview);

	populate_log_tree(lv);

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (lv->treeview));
	g_signal_connect (G_OBJECT (sel), "changed",
			G_CALLBACK (log_select_cb),
			lv);
	g_signal_connect (G_OBJECT(lv->treeview), "row-activated",
			G_CALLBACK(log_row_activated_cb),
			lv);
	pidgin_set_accessible_label(lv->treeview, lv->label);

	/* A fancy little box ************/
	vbox = gtk_vbox_new(FALSE, PIDGIN_HIG_BOX_SPACE);
	gtk_paned_add2(GTK_PANED(pane), vbox);

	/* Viewer ************/
	frame = pidgin_create_imhtml(FALSE, &lv->imhtml, NULL, NULL);
	gtk_widget_set_name(lv->imhtml, "pidginlog_imhtml");
	gtk_widget_set_size_request(lv->imhtml, 320, 200);
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
	gtk_widget_show(frame);

	/* Search box **********/
	hbox = gtk_hbox_new(FALSE, PIDGIN_HIG_BOX_SPACE);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	lv->entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), lv->entry, TRUE, TRUE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_FIND);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_signal_connect(GTK_ENTRY(lv->entry), "activate", G_CALLBACK(search_cb), lv);
	g_signal_connect(GTK_BUTTON(button), "activate", G_CALLBACK(search_cb), lv);
	g_signal_connect(GTK_BUTTON(button), "clicked", G_CALLBACK(search_cb), lv);

#if GTK_CHECK_VERSION(2,2,0)
	/* Show most recent log **********/
	path_to_first_log = gtk_tree_path_new_from_string("0:0");
	if (path_to_first_log)
	{
		gtk_tree_view_expand_to_path(GTK_TREE_VIEW(lv->treeview), path_to_first_log);
		gtk_tree_selection_select_path(sel, path_to_first_log);
		gtk_tree_path_free(path_to_first_log);
	}
#endif

	gtk_widget_show_all(lv->window);
	syslog_viewer = lv;
}

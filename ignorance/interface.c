/*
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <gtkutils.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "ignorance.h"
#include "ignorance_internal.h"

GtkWidget* create_uiinfo (GPtrArray *levels) {
	GtkWidget *frame, *table, *hbox, *label;
	GtkWidget *scrolledwindow, *levelView, *button;

	rule_selected=TRUE;
	vbox1=gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox1);

	vbox2=gtk_vbox_new(FALSE, PURPLE_HIG_BOX_SPACE);
	gtk_widget_show(vbox2);
	gtk_box_pack_start(GTK_BOX (vbox1), vbox2, TRUE, TRUE, 0);

	scrolledwindow=gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_box_pack_start(GTK_BOX (vbox2), scrolledwindow, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolledwindow),
								   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (scrolledwindow),
										GTK_SHADOW_IN);

	store=gtk_tree_store_new(NUM_COLUMNS,G_TYPE_STRING,G_TYPE_STRING);

	levelView=gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

	renderer=gtk_cell_renderer_text_new ();
	column=gtk_tree_view_column_new_with_attributes ("Levels", renderer,
													 "text", LEVEL_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW (levelView), column);
	column=gtk_tree_view_column_new_with_attributes("Rules", renderer, "text",
													RULE_COLUMN,NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (levelView), column);  

	load_form_with_levels(GTK_TREE_VIEW(levelView), levels);

	gtk_widget_show(levelView);
	gtk_container_add(GTK_CONTAINER(scrolledwindow), levelView);

	sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(levelView));
	gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);  

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

	button=pidgin_pixbuf_button_from_stock(_("Create new rule"),
										GTK_STOCK_ADD, PIDGIN_BUTTON_HORIZONTAL);
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
	g_signal_connect((gpointer) button, "clicked",
					 G_CALLBACK (on_levelAdd_clicked), levelView);

	button=pidgin_pixbuf_button_from_stock(_("Create new group"),
										GTK_STOCK_ADD, PIDGIN_BUTTON_HORIZONTAL);
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
	g_signal_connect ((gpointer) button, "clicked",
					  G_CALLBACK (on_groupAdd_clicked), levelView);

	button=pidgin_pixbuf_button_from_stock(_("Save changes"), GTK_STOCK_YES,
										 PIDGIN_BUTTON_HORIZONTAL);
	gtk_widget_show (button);
	gtk_box_pack_start(GTK_BOX (hbox), button, TRUE, TRUE, 0);
	g_signal_connect((gpointer) button, "clicked",
					  G_CALLBACK (on_levelEdit_clicked), levelView);

/* XXX: The stock-icon for levelDel doesn't show, because the text is
 * set from callback.c. Can we do with just `Remove' for the text
 * and not updating as the selection in the tree changes?
 */
	levelDel=pidgin_pixbuf_button_from_stock(_("Remove rule"), GTK_STOCK_REMOVE,
										   PIDGIN_BUTTON_HORIZONTAL);
	gtk_widget_show (levelDel);
	gtk_box_pack_start (GTK_BOX (hbox), levelDel, TRUE, TRUE, 0);
	g_signal_connect((gpointer) levelDel, "clicked",
					 G_CALLBACK (on_levelDel_clicked), levelView);

	table=gtk_table_new(3, 2, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(table), PURPLE_HIG_BOX_SPACE);
	gtk_table_set_col_spacings(GTK_TABLE(table), PURPLE_HIG_BOX_SPACE);
	gtk_table_set_row_spacings(GTK_TABLE(table), PURPLE_HIG_BOX_SPACE);
	gtk_widget_show (table);
	gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, TRUE, 0);

	label=gtk_label_new_with_mnemonic(_("Name: "));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

	rulename=gtk_entry_new ();
	gtk_widget_show(rulename);
	gtk_table_attach_defaults(GTK_TABLE (table), rulename, 1, 2, 0, 1);

	label=gtk_label_new_with_mnemonic(_("Filter: "));
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);

	filtervalue=gtk_entry_new ();
	gtk_widget_show(filtervalue);
	gtk_table_attach_defaults(GTK_TABLE (table), filtervalue, 1, 2, 1, 2);

	hbox=gtk_hbox_new (FALSE, 0);
	gtk_widget_show(hbox);
	gtk_table_attach_defaults(GTK_TABLE (table), hbox, 0, 2, 2, 3);

	enabled_cb=gtk_check_button_new_with_mnemonic(_("Enabled"));
	gtk_widget_show(enabled_cb);
	gtk_box_pack_start(GTK_BOX(hbox),enabled_cb,FALSE,FALSE,0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enabled_cb),TRUE);

#ifdef HAVE_REGEX_H
	regex_cb=gtk_check_button_new_with_mnemonic (_("Regular Expression"));
	gtk_widget_show(regex_cb);
	gtk_box_pack_start (GTK_BOX (hbox), regex_cb, FALSE, FALSE, 0);
#endif

	repeat_cb=gtk_check_button_new_with_mnemonic (_("Repeat"));
	gtk_widget_set_sensitive(repeat_cb,FALSE);
	gtk_widget_show(repeat_cb);
	gtk_box_pack_start(GTK_BOX (hbox), repeat_cb, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (repeat_cb), FALSE);

	frame=gtk_frame_new (NULL);
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX (vbox2), frame, FALSE, TRUE, 0);

	table=gtk_table_new(4, 2, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(table), PURPLE_HIG_BOX_SPACE);
	gtk_table_set_col_spacings(GTK_TABLE(table), PURPLE_HIG_BOX_SPACE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);

	hbox=gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 0, 2, 0, 1);

	filter_cb=gtk_check_button_new_with_label(_("Filter"));
	gtk_widget_show(filter_cb);
	gtk_box_pack_start(GTK_BOX (hbox), filter_cb, FALSE, FALSE, 0);

	ignore_cb=gtk_check_button_new_with_label(_("Ignore"));
	gtk_widget_show(ignore_cb);
	gtk_box_pack_start(GTK_BOX (hbox), ignore_cb, FALSE, FALSE, 0);

	message_cb = gtk_check_button_new_with_label(_("Send Message"));
	gtk_widget_show(message_cb);
	gtk_table_attach(GTK_TABLE(table), message_cb, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);

	message_entry=gtk_entry_new();
	gtk_widget_set_sensitive(message_entry,FALSE);
	gtk_widget_show(message_entry);
	gtk_table_attach_defaults(GTK_TABLE(table), message_entry, 1, 2, 1, 2);

	sound_cb = gtk_check_button_new_with_label(_("Play sound"));
	gtk_widget_show(sound_cb);
	gtk_table_attach(GTK_TABLE(table), sound_cb, 0, 1, 2, 3, GTK_FILL, 0, 0, 0);

	hbox=gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 1, 2, 2, 3);

	sound_entry=gtk_entry_new();
	gtk_widget_set_sensitive(sound_entry,FALSE);
	gtk_widget_show(sound_entry);
	gtk_box_pack_start(GTK_BOX(hbox),sound_entry,FALSE,FALSE,0);

	sound_browse=gtk_button_new_with_label(_("Browse"));
	gtk_widget_set_sensitive(sound_browse,FALSE);
	gtk_widget_show(sound_browse);
	gtk_box_pack_start(GTK_BOX(hbox),sound_browse,FALSE,FALSE,0);

	execute_cb=gtk_check_button_new_with_label(_("Execute command"));
	gtk_widget_show(execute_cb);
	gtk_table_attach(GTK_TABLE(table), execute_cb, 0, 1, 3, 4, GTK_FILL, 0, 0, 0);

	execute_entry=gtk_entry_new();
	gtk_widget_set_sensitive(execute_entry,FALSE);
	gtk_widget_show(execute_entry);
	gtk_table_attach_defaults(GTK_TABLE(table), execute_entry, 1, 2, 3, 4);

	label=gtk_label_new(_("Take action"));
	gtk_widget_show(label);
	gtk_frame_set_label_widget(GTK_FRAME (frame), label);

	frame=gtk_frame_new (NULL);
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX (vbox2), frame, FALSE, TRUE, 0);

	table=gtk_table_new (3, 2, FALSE);
	gtk_widget_show(table);
	gtk_container_set_border_width(GTK_CONTAINER(table), PURPLE_HIG_BOX_SPACE);
	gtk_table_set_col_spacings(GTK_TABLE(table), PURPLE_HIG_BOX_SPACE);
	gtk_container_add(GTK_CONTAINER (frame), table);

	im_type_cb=gtk_check_button_new_with_mnemonic(_("IM Text"));
	gtk_widget_show(im_type_cb);
	gtk_table_attach(GTK_TABLE (table), im_type_cb, 0, 1, 0, 1,
					 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

	chat_type_cb=gtk_check_button_new_with_mnemonic(_("Chat Text"));
	gtk_widget_show(chat_type_cb);
	gtk_table_attach(GTK_TABLE (table), chat_type_cb, 1, 2, 0, 1,
					 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (chat_type_cb), TRUE);

	username_type_cb=gtk_check_button_new_with_mnemonic(_("User names"));
	gtk_widget_show(username_type_cb);
	gtk_table_attach(GTK_TABLE (table), username_type_cb, 0, 1, 1, 2,
					 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
					 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

	enterleave_type_cb=gtk_check_button_new_with_mnemonic (_("Enter/Leave"));
	gtk_widget_show(enterleave_type_cb);
	gtk_table_attach(GTK_TABLE (table), enterleave_type_cb, 1, 2, 1, 2,
					 (GtkAttachOptions) (GTK_FILL),
					 (GtkAttachOptions) (0), 0, 0);

	invite_type_cb=gtk_check_button_new_with_mnemonic (_("Invitations"));
	gtk_widget_show(invite_type_cb);
	gtk_table_attach(GTK_TABLE (table), invite_type_cb, 0, 1, 2, 3,
					 (GtkAttachOptions) (GTK_FILL),
					 (GtkAttachOptions) (0), 0, 0);

	label=gtk_label_new (_("Filter"));
	gtk_widget_show(label);
	gtk_frame_set_label_widget(GTK_FRAME (frame), label);

	g_signal_connect(GTK_WIDGET(vbox1), "destroy", G_CALLBACK(save_conf),NULL);
	g_signal_connect((gpointer)sel, "changed",
					 G_CALLBACK (on_levelView_row_activated), NULL);

	g_signal_connect((gpointer) filter_cb, "toggled",
					 G_CALLBACK (on_filter_cb_toggled), NULL);

	g_signal_connect((gpointer) ignore_cb, "toggled",
					 G_CALLBACK (on_ignore_cb_toggled), NULL);

	g_signal_connect((gpointer) message_cb, "toggled",
					 G_CALLBACK (on_message_cb_toggled), NULL);

	g_signal_connect((gpointer) sound_cb, "toggled",
					 G_CALLBACK (on_sound_cb_toggled), NULL);

	g_signal_connect((gpointer) execute_cb, "toggled",
					 G_CALLBACK (on_execute_cb_toggled), NULL);

	g_signal_connect((gpointer) sound_browse, "clicked",
					 G_CALLBACK (on_sound_browse_clicked), NULL);

	return vbox1;
}

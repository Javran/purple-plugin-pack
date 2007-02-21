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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef IGNORANCE_INTERFACE_H
#define IGNORANCE_INTERFACE_H
 
GtkWidget *vbox1;
GtkWidget *vbox2;
GtkWidget *levelDel;
GtkWidget *rulename;
GtkWidget *filtervalue;
GtkWidget *repeat_cb;
GtkWidget *regex_cb;
GtkWidget *im_type_cb;
GtkWidget *chat_type_cb;
GtkWidget *username_type_cb;
GtkWidget *enterleave_type_cb;
GtkWidget *invite_type_cb;
GtkWidget *enabled_cb;
GtkWidget *filter_cb;
GtkWidget *ignore_cb;
GtkWidget *message_cb, *message_entry;
GtkWidget *sound_cb, *sound_entry, *sound_browse;

GtkWidget *execute_cb, *execute_entry, *execute_browse;

GtkCellRenderer *renderer;
GtkTreeViewColumn *column;
GtkTreeStore *store;
GtkTreeSelection *sel;
gboolean rule_selected;

GtkWidget* create_uiinfo(GPtrArray *levels);

#endif

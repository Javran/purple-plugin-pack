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

#ifndef IGNORANCE_CALLBACKS_H
#define IGNORANCE_CALLBACKS_H

#include <gtk/gtk.h>

#ifdef HAVE_REGEX_H
# include <regex.h>
#endif

enum{
	LEVEL_COLUMN,
	RULE_COLUMN,
	NUM_COLUMNS
};

gboolean on_levelView_row_activated  (GtkTreeSelection *sel, gpointer user_data);

void on_levelAdd_clicked (GtkButton *button, gpointer user_data);

void on_groupAdd_clicked (GtkButton *button, gpointer user_data);

void on_levelEdit_clicked (GtkButton *button, gpointer user_data);

void on_levelDel_clicked (GtkButton *button, gpointer user_data);

void on_sound_browse_clicked (GtkButton *button, gpointer user_data);

/*
void on_execute_browse_clicked (GtkButton *button, gpointer user_data);
*/

void on_filter_cb_toggled (GtkButton *button, gpointer user_data);

void on_ignore_cb_toggled (GtkButton *button, gpointer user_data);

void on_message_cb_toggled (GtkButton *button, gpointer user_data);

void on_sound_cb_toggled (GtkButton *button, gpointer user_data);

void on_execute_cb_toggled (GtkButton *button, gpointer user_data);

gboolean load_form_with_levels (GtkTreeView *tree, GPtrArray *levels);

#endif

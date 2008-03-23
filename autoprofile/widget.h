/*--------------------------------------------------------------------------*
 * AUTOPROFILE                                                              *
 *                                                                          *
 * A Purple away message and profile manager that supports dynamic text       *
 *                                                                          *
 * AutoProfile is the legal property of its developers.  Please refer to    *
 * the COPYRIGHT file distributed with this source distribution.            *
 *                                                                          *
 * This program is free software; you can redistribute it and/or modify     *
 * it under the terms of the GNU General Public License as published by     *
 * the Free Software Foundation; either version 2 of the License, or        *
 * (at your option) any later version.                                      *
 *                                                                          *
 * This program is distributed in the hope that it will be useful,          *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 * GNU General Public License for more details.                             *
 *                                                                          *
 * You should have received a copy of the GNU General Public License        *
 * along with this program; if not, write to the Free Software              *
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA *
 *--------------------------------------------------------------------------*/

#ifndef _AP_WIDGET_H_
#define _AP_WIDGET_H_

#include "component.h"

#include "gtkgaim.h"
#include "gtkutils.h"

/* The heart of everything */
struct widget {
  char *wid;
  char *alias;
  struct component *component;
  GHashTable *data;
};

void ap_widget_init ();
void ap_widget_start ();
void ap_widget_finish ();
void ap_widget_gtk_start ();
void ap_widget_gtk_finish ();

/* Basic functions */
gboolean ap_widget_has_content_changed ();
GList *ap_widget_get_widgets ();

struct widget *ap_widget_find (const gchar *);
struct widget *ap_widget_find_by_identifier (const gchar *);

struct widget *ap_widget_create (struct component *);
void ap_widget_delete (struct widget *);

// TRUE if rename succeeds, FALSE otherwise
gboolean ap_widget_rename (struct widget *, const gchar *);

/* GUI functions */
GtkWidget *ap_widget_get_config_page ();
void ap_widget_prefs_updated (struct widget *);
GtkWidget *get_widget_list (GtkWidget *, GtkTreeSelection **);
void done_with_widget_list ();

/* Widget data galore! */
void     ap_widget_set_data (struct widget *, int, gpointer);
gpointer ap_widget_get_data (struct widget *, int);

/* Widget preferences galore! */
gchar *ap_prefs_get_pref_name (struct widget *, const char *);
GtkWidget *ap_prefs_checkbox (struct widget *, const char *, const char *,
  GtkWidget *);
GtkWidget *ap_prefs_dropdown_from_list (struct widget *, GtkWidget *,
  const gchar *, PurplePrefType, const char *, GList *);
GtkWidget *ap_prefs_labeled_entry (struct widget *, GtkWidget *page, 
  const gchar *, const char *, GtkSizeGroup *);
GtkWidget *ap_prefs_labeled_spin_button (struct widget *, GtkWidget *,
  const gchar *, const char *, int, int, GtkSizeGroup *);

void ap_prefs_add_bool (struct widget *, const char *name, gboolean value);
void ap_prefs_add_int (struct widget *, const char *name, int value);
void ap_prefs_add_none (struct widget *, const char *name);
void ap_prefs_add_string (struct widget *, const char *, const char *);
void ap_prefs_add_string_list (struct widget *, const char *, GList *);

gboolean      ap_prefs_get_bool (struct widget *, const char *name);
int           ap_prefs_get_int (struct widget *, const char *name);
const char *  ap_prefs_get_string (struct widget *, const char *name);
GList *       ap_prefs_get_string_list (struct widget *, const char *name);

void ap_prefs_set_bool (struct widget *, const char *name, gboolean value);
void ap_prefs_set_int (struct widget *, const char *name, int value);
void ap_prefs_set_string (struct widget *, const char *name, const char *);
void ap_prefs_set_string_list (struct widget *, const char *, GList *);

#endif /* _AP_WIDGET_H_ */

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

#include "component.h"

static GHashTable *refresh_timeouts = NULL;

/*---------- HTTP: HTTP requested Data ----------*/
static void http_response (PurpleUtilFetchUrlData *reuqest_data, gpointer data, const char *c, gsize len, const gchar *error_message)
{
  struct widget *w;
  w = (struct widget *) data;

  // Invalid URL!
  if (c == NULL) {
    ap_prefs_set_string (w, "http_data", 
      _("[AutoProfile error: Invalid URL or no internet connection]"));
    return;
  }

  w = (struct widget *) data;
  ap_prefs_set_string (w, "http_data", c);
}

static char* http_generate (struct widget *w)
{
  const char *result, *url;

  url = ap_prefs_get_string (w, "http_url");
  if (!url || url[0] == '\0') {
    return strdup (_("[AutoProfile error: No URL specified]"));
  }

  result = ap_prefs_get_string (w, "http_data");
  if (result == NULL) return strdup ("");
  return strdup (result);
}

static gboolean http_refresh_update (gpointer user_data)
{
  struct widget *w;
  char *http_url;

  w = (struct widget *) user_data;
  http_url = strdup (ap_prefs_get_string (w, "http_url"));

  if( http_url && (http_url[0] != '\0') ) {
    purple_util_fetch_url(http_url, TRUE, NULL, FALSE, http_response, w);
  } else {
    ap_prefs_set_string (w, "http_data", "");
  }

  free (http_url);
  return TRUE;
}

static void http_load (struct widget *w)
{
  gpointer http_refresh_timeout;

  if (refresh_timeouts == NULL) {
    refresh_timeouts = g_hash_table_new (NULL, NULL);
  }

  http_refresh_update (w);
  http_refresh_timeout = GINT_TO_POINTER (g_timeout_add (
    ap_prefs_get_int (w, "http_refresh_mins") * 60 * 1000,
    http_refresh_update, w));
  g_hash_table_insert (refresh_timeouts, w, http_refresh_timeout);
}

static void http_unload (struct widget *w)
{
  gpointer http_refresh_timeout;

  http_refresh_timeout = g_hash_table_lookup (refresh_timeouts, w);
  g_source_remove (GPOINTER_TO_INT (http_refresh_timeout));
  g_hash_table_remove (refresh_timeouts, w);
}

static void http_init (struct widget *w)
{
  ap_prefs_add_string (w, "http_url", "");
  ap_prefs_add_string (w, "http_data", "");
  ap_prefs_add_int (w, "http_refresh_mins", 1);
}

static gboolean http_url_update (GtkWidget *widget, GdkEventFocus *evt, 
  gpointer data)
{
  struct widget *w = (struct widget *) data;
  ap_prefs_set_string (w, "http_url",
    gtk_entry_get_text (GTK_ENTRY (widget)));

  return FALSE;
}

static gboolean http_refresh_mins_update (GtkWidget *widget, gpointer data)
{
  struct widget *w;
  gpointer timeout;
  int minutes;

  minutes = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));

  w = (struct widget *) data;
  ap_prefs_set_int (w, "http_refresh_mins", minutes);

  // Kill the current timer and run a new one
  timeout = g_hash_table_lookup (refresh_timeouts, w);
  g_source_remove (GPOINTER_TO_INT(timeout));
  timeout = GINT_TO_POINTER (g_timeout_add (minutes * 60 * 1000,
    http_refresh_update, w));
  g_hash_table_replace (refresh_timeouts, w, timeout);

  return FALSE;
}

static void http_data_update (GtkWidget *w, gpointer data) {
  http_refresh_update (data);
}

static GtkWidget *http_menu (struct widget *w)
{
  GtkWidget *ret = gtk_vbox_new (FALSE, 5);
  GtkWidget *label, *hbox, *button, *spinner;
  GtkWidget *http_url_entry;

  label = gtk_label_new (_("Select URL with source content"));
  gtk_box_pack_start (GTK_BOX (ret), label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (ret), hbox, FALSE, FALSE, 0);

  // URL Entry
  http_url_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), http_url_entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (http_url_entry),
    ap_prefs_get_string (w, "http_url"));
  g_signal_connect (G_OBJECT (http_url_entry), "focus-out-event",
                    G_CALLBACK (http_url_update), w);

  // Update Now!
  button = gtk_button_new_with_label (_("Fetch page now!"));
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (http_data_update), w);

  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (ret), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Delay"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  spinner = gtk_spin_button_new_with_range (1, 60, 1);
  gtk_box_pack_start (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinner),
    ap_prefs_get_int (w, "http_refresh_mins"));
  g_signal_connect (G_OBJECT (spinner), "value-changed",
                    G_CALLBACK (http_refresh_mins_update), w);

  label = gtk_label_new (_("minutes between page fetches"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  return ret;
}

struct component http =
{
  N_("Webpage"),
  N_("Data fetched from an internet URL using HTTP"),
  "Webpage",
  &http_generate,
  &http_init,
  &http_load,
  &http_unload,
  NULL,
  &http_menu
};


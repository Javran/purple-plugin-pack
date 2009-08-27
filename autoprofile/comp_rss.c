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

#include "comp_rss.h"
#include "autoprofile.h"

#include "gtkimhtml.h"

#include <ctype.h>

static GtkWidget *entry_username = NULL;
static GtkWidget *entry_url = NULL;

GHashTable *rss_entries = NULL;
static GHashTable *rss_timeouts = NULL;
GStaticMutex rss_mutex = G_STATIC_MUTEX_INIT;

/* Core functions */
static char *get_rss_data (struct widget *w, const char *field, int index, 
  struct tm **time) 
{
  GList *tmp;
  const struct rss_entry *e;
  char *ret;
  int max;

  g_static_mutex_lock (&rss_mutex);
  tmp = (GList *) g_hash_table_lookup (rss_entries, w);

  if (index < 0 ) {
    g_static_mutex_unlock (&rss_mutex);
    return strdup (_("[ERROR: Invalid entry number]")); 
  }

  if (tmp == NULL) {
    g_static_mutex_unlock (&rss_mutex);
    return strdup (_("[ERROR: No data, invalid URL/account?]"));
  }

  if (index != 0) {
    while (index-- != 1) {
      tmp = tmp->next;
      if (tmp == NULL) {
        g_static_mutex_unlock (&rss_mutex);
        return strdup (_("[ERROR: Insufficient number of entries]"));
      }
    }
  }
 
  e = (struct rss_entry *) tmp->data;

  if (!strcmp (field, "link")) {
    if (e->url)
      ret = strdup (e->url);
    else
      ret = NULL;
  } else if (!strcmp (field, "title")) {
    if (e->title)
      ret = strdup (e->title);
    else
      ret = NULL;
  } else if (!strcmp (field, "entry")) {
    if (e->entry) {
      max = ap_prefs_get_int (w, "entry_limit");
      ret = strdup (e->entry);
      if (max < g_utf8_strlen (ret, -1)) {
        gchar *tmp = g_utf8_offset_to_pointer (ret, max);    
        *tmp = '\0';
      } 
    } else {
      ret = NULL;
    }
  } else if (!strcmp (field, "time")) {
    *time = e->t;
    ret = NULL;
  } else {
    ret = NULL;
  }

  g_static_mutex_unlock (&rss_mutex);
  return ret;   
}

static char *rss_generate (struct widget *w)
{
  GString *output;
  gchar *result;

  char *tmp, *time_tmp;
  int state;
  int count;
  const char *format;
  char fmt_char [3];
  struct tm *time;
  
  fmt_char[0] = '%';
  fmt_char[2] = '\0';
  
  format = ap_prefs_get_string (w, "format");
  output = g_string_new ("");
  time_tmp = (char *)malloc (sizeof(char)*AP_SIZE_MAXIMUM);
  
  state = 0;
  count = 0;

  while (*format) {
    if (state == 1) {
      if (isdigit (*format)) {
        count = (count * 10) + (int) *format - 48;
        format++;
      } else {
        switch (*format) {
          case 'H':
          case 'I':
          case 'p':
          case 'M':
          case 'S':
          case 'a':
          case 'A':
          case 'b':
          case 'B':
          case 'm':
          case 'd':
          case 'j':
          case 'W':
          case 'w':
          case 'y':
          case 'Y':
          case 'z':
            time = NULL;
            tmp = get_rss_data (w, "time", count, &time);
            if (time) {
              fmt_char[1] = *format;
              strftime (time_tmp, AP_SIZE_MAXIMUM, fmt_char, time);
              g_string_append_printf (output, "%s", time_tmp);
            } 
            break;
          case 'l':
            tmp = get_rss_data (w, "link", count, NULL);
            if (tmp) {
              g_string_append_printf (output, "%s", tmp);
              free (tmp);
            } 
            break;
          case 't':
            tmp = get_rss_data (w, "title", count, NULL);
            if (tmp) {
              g_string_append_printf (output, "%s", tmp);
              free (tmp);
            }
            break;
          case 'e':
            tmp = get_rss_data (w, "entry", count, NULL);
            if (tmp) {
              g_string_append_printf (output, "%s", tmp);
              free (tmp);
            }
            break;
          case '%':
            g_string_append_printf (output, "%c", *format);
            break;
          default:
            g_string_append_unichar (output, g_utf8_get_char (format));
            break;
        }
        format = g_utf8_next_char (format);
        state = 0;
      }
    } else {
      if (*format == '%') {
        state = 1;
        count = 0;
      } else {
        g_string_append_unichar (output, g_utf8_get_char (format));
      }
      format = g_utf8_next_char (format);
    }
  }

  result = output->str;
  g_string_free (output, FALSE);
  return result;
}

static gboolean rss_update (gpointer data)
{
  parse_rss ((struct widget *) data);
  return TRUE;
}

static void rss_load (struct widget *w)
{
  gpointer rss_timeout;

  g_static_mutex_lock (&rss_mutex);
  if (!rss_entries) {
    rss_entries = g_hash_table_new (NULL, NULL);
  }
  if (!rss_timeouts) {
    rss_timeouts = g_hash_table_new (NULL, NULL);
  }

  rss_timeout = GINT_TO_POINTER (g_timeout_add (
    ap_prefs_get_int (w, "update_rate") * 60 * 1000,
    rss_update, w));
  g_hash_table_insert (rss_timeouts, w, rss_timeout);

  g_static_mutex_unlock (&rss_mutex);

  rss_update (w);
}

static void rss_unload (struct widget *w)
{
  gpointer rss_timeout;

  g_static_mutex_lock (&rss_mutex);
  rss_timeout = g_hash_table_lookup (rss_timeouts, w);
  g_source_remove (GPOINTER_TO_INT (rss_timeout));
  g_hash_table_remove (rss_timeouts, w);

  g_static_mutex_unlock (&rss_mutex);
}

static void rss_init (struct widget *w)
{
  ap_prefs_add_int (w, "type", RSS_XANGA);
  ap_prefs_add_string (w, "location", "");
  ap_prefs_add_string (w, "username", "");
  ap_prefs_add_string (w, "format", 
    "My <a href=\"%l\">blog</a> was most recently updated on "
    "%1B %1d at %I:%M %p");
  ap_prefs_add_int (w, "update_rate", 5);
  ap_prefs_add_int (w, "entry_limit", 1000);
}

/* GUI functions */
static gboolean update_refresh_rate (GtkWidget *widget, GdkEventFocus *evt, 
                              struct widget *w)
{
  gpointer timeout;
  int minutes;

  minutes = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));
  ap_prefs_set_int (w, "update_rate", minutes);
   
  // Kill the current timer and run a new one
  g_static_mutex_lock (&rss_mutex);
  timeout = g_hash_table_lookup (rss_timeouts, w);
  g_source_remove (GPOINTER_TO_INT(timeout));
  timeout = GINT_TO_POINTER (g_timeout_add (minutes * 60 * 1000,
    rss_update, w));
  g_hash_table_replace (rss_timeouts, w, timeout);
  g_static_mutex_unlock (&rss_mutex);

  return FALSE;
}

static void rss_data_update (GtkWidget *widget, struct widget *w)
{
  rss_update (w);
}

static void sensitivity_cb (const char *name, PurplePrefType type,
  gconstpointer val, gpointer data) 
{
  int real_val = GPOINTER_TO_INT (val);

  if (real_val == RSS_XANGA || real_val == RSS_LIVEJOURNAL) {
    gtk_widget_set_sensitive (entry_username, TRUE);
    gtk_widget_set_sensitive (entry_url, FALSE);
  } else {
    gtk_widget_set_sensitive (entry_username, FALSE);
    gtk_widget_set_sensitive (entry_url, TRUE);
  }
}

static GtkWidget *entry;

static void event_cb (GtkWidget *widget, struct widget *w)
{
  ap_prefs_set_string (w, "format", 
                  gtk_imhtml_get_markup (GTK_IMHTML(entry)));
}
static void formatting_toggle_cb (GtkIMHtml *imhtml, 
                GtkIMHtmlButtons buttons, struct widget *w)
{
  ap_prefs_set_string (w, "format", 
                  gtk_imhtml_get_markup (GTK_IMHTML(entry)));

}

static void formatting_clear_cb (GtkIMHtml *imhtml,
               struct widget *w)
{
  ap_prefs_set_string (w, "format", 
                  gtk_imhtml_get_markup (GTK_IMHTML(entry)));
}

static GtkWidget *rss_menu (struct widget *w)
{
  GtkWidget *ret;
  GList *options;
  GtkWidget *label, *hbox, *button, *spinner, *sw;
  GtkWidget *entry_window, *toolbar;
  GtkTextBuffer *text_buffer;
  
  int value;
  gchar *pref;

  ret = gtk_vbox_new (FALSE, 5);

  /* Format string */
  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL(label), "<b>Format string for output</b>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_box_pack_start (GTK_BOX(ret), label, FALSE, FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX(ret), hbox, TRUE, TRUE, 0);
  
  entry_window = pidgin_create_imhtml (TRUE, &entry, &toolbar, &sw);
  gtk_box_pack_start (GTK_BOX (hbox), entry_window, TRUE, TRUE, 0);
  gtk_imhtml_append_text_with_images (GTK_IMHTML(entry),
                  ap_prefs_get_string (w, "format"),
                  0, NULL);
  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (entry));
  g_signal_connect (G_OBJECT (text_buffer), "changed",
                    G_CALLBACK (event_cb), w);
  g_signal_connect_after(G_OBJECT(entry), "format_function_toggle",
           G_CALLBACK(formatting_toggle_cb), w);
  g_signal_connect_after(G_OBJECT(entry), "format_function_clear",
           G_CALLBACK(formatting_clear_cb), w); 

  label = gtk_label_new (_(
    "The following options can be specified with a numerical modifier\n"
    "(i.e. \"%e\" can also be written \"%1e\" or \"%2e\").  The optional\n"
    "number specifies which entry to get the data for. \"1\" refers to the\n"
    "most recent entry, \"2\" refers to the second-most recent entry, and so\n"
    "forth.  \"1\" is used if no number is specified.\n\n"
    "%e\tStarting text of the entry.\n"
    "%l\tLink to the specific entry.\n"
    "%t\tTitle of entry (Xanga incompatible)\n"

    "\nTime of entry:\n"
    "%H\thour of entry(24-hour clock)\n"
    "%I\thour (12-hour clock)\n"
    "%p\tAM or PM\n"
    "%M\tminute\n"
    "%S\tsecond\n"
    "%a\tabbreviated weekday name\n"
    "%A\tfull weekday name\n"
    "%b\tabbreviated month name\n"
    "%B\tfull month name\n"
    "%m\tmonth (numerical)\n"
    "%d\tday of the month\n"
    "%j\tday of the year\n"
    "%W\tweek number of the year\n"
    "%w\tweekday (numerical)\n"
    "%y\tyear without century\n"
    "%Y\tyear with century\n"
    "%z\ttime zone name, if any\n"
    "%%\t%"));

  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  sw = gtk_scrolled_window_new (NULL,NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (hbox), sw, TRUE, TRUE , 0);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(sw), label);

  /* Type/URL/Username selection */
  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL(label), "<b>RSS/Blog location</b>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_box_pack_start (GTK_BOX(ret), label, FALSE, FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX(ret), hbox, FALSE, FALSE, 0);

  /* Dropdown */
  options = g_list_append (NULL, (char *) _("Xanga"));
  options = g_list_append (options, GINT_TO_POINTER(RSS_XANGA));
  options = g_list_append (options, (char *) _("LiveJournal"));
  options = g_list_append (options, GINT_TO_POINTER(RSS_LIVEJOURNAL));
  options = g_list_append (options, (char *) _("RSS 2.0"));
  options = g_list_append (options, GINT_TO_POINTER(RSS_2));
  ap_prefs_dropdown_from_list (w, hbox, NULL, PURPLE_PREF_INT, "type", options);
  g_list_free (options);
  
  pref = ap_prefs_get_pref_name (w, "type");
  purple_prefs_connect_callback (ap_get_plugin_handle (), pref,
    sensitivity_cb, w);
  free (pref);

  /* Username/URL fields */
  entry_username = ap_prefs_labeled_entry (w, hbox, _("Username:"),
    "username", NULL);
  entry_url = ap_prefs_labeled_entry (w, hbox, _("URL of feed:"),
    "location", NULL);

  value = ap_prefs_get_int (w, "type");
  if (value == RSS_XANGA || value == RSS_LIVEJOURNAL) {
    gtk_widget_set_sensitive (entry_username, TRUE);
    gtk_widget_set_sensitive (entry_url, FALSE);
  } else {
    gtk_widget_set_sensitive (entry_username, FALSE);
    gtk_widget_set_sensitive (entry_url, TRUE);
  }

  /* Other options */
  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL(label), "<b>Other options</b>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_box_pack_start (GTK_BOX(ret), label, FALSE, FALSE, 0);

  /* # of chars to display from description */
  ap_prefs_labeled_spin_button (w, ret, 
    "Max characters to show in entry (%e)", "entry_limit", 1,
    AP_SIZE_MAXIMUM - 1, NULL);

  /* Update rate selection */
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (ret), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Minutes between checks for updates:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  spinner = gtk_spin_button_new_with_range (1, 60, 1);
  gtk_box_pack_start (GTK_BOX(hbox), spinner, FALSE, FALSE, 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinner),
    ap_prefs_get_int (w, "update_rate"));
  g_signal_connect (G_OBJECT (spinner), "value-changed",
                    G_CALLBACK (update_refresh_rate), w);

  button = gtk_button_new_with_label ("Fetch data now!");
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (rss_data_update), w);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

  return ret;
}

/* Le end */
struct component rss =
{
  N_("RSS / Blogs"),
  N_("Information taken from an RSS feed (Xanga and LiveJournal capable)"),
  "RSS",
  &rss_generate,
  &rss_init,
  &rss_load,
  &rss_unload,
  NULL,
  &rss_menu
};


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

#include "autoprofile.h"

#include "gtkdialogs.h"
#include "gtkimhtml.h"
#include "gtkprefs.h"

/* VARIABLE DEFINITIONS */
static guint queue_pref_cb = 0;
static guint sound_pref_cb = 0;
static gboolean ap_previously_away = FALSE;

/* The list containing data on generated profiles / status messages */
static GtkListStore *message_list = NULL;

/* The general window */
static GtkWidget *dialog = NULL;

/* Progress bars */
typedef struct _ap_progress_bar {
  APUpdateType type;
  GtkWidget *bar;
  guint timeout;
} APProgressBar;
static GHashTable *progress_bars = NULL;

/*--------------------------------------------------------------------------*
 * Callback functions                                                       *
 *--------------------------------------------------------------------------*/
static void hide_cb (GtkButton *button, gpointer data) {
  gtk_widget_hide_all (dialog);
}

static void queue_cb (
  const char *name, PurplePrefType type, gconstpointer val, gpointer data) 
{
  ap_update_queueing ();
}

static void sound_cb (
  const char *name, PurplePrefType type, gconstpointer val, gpointer data) 
{
  GtkWidget *button;
  gboolean value;

  button = (GtkWidget *) data;
  value = purple_prefs_get_bool ("/core/sound/while_away");

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button), value);
}

static void update_summary_visibility ()
{
  const gchar *summary_pref;

  // Decide whether or not to show window
  summary_pref = purple_prefs_get_string (
    "/plugins/gtk/autoprofile/show_summary");

  if (!strcmp (summary_pref, "always")) {
    gtk_widget_show_all (dialog);
  } else if (!strcmp (summary_pref, "away") && ap_is_currently_away ()) {
    gtk_widget_show_all (dialog);
  } else {
    gtk_widget_hide_all (dialog);
  }

  ap_previously_away = ap_is_currently_away ();
}

/*--------------------------------------------------------------------------*
 * Displayed message stuff                                                  *
 *--------------------------------------------------------------------------*/
static void display_diff_msg (GtkTreeSelection *select, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  const gchar *string;
  GtkWidget *imhtml = (GtkWidget *) data;
 
  if (gtk_tree_selection_get_selected (select, &model, &iter)) 
  {
    gtk_tree_model_get (model, &iter, 3, &string, -1);
    gtk_imhtml_clear (GTK_IMHTML(imhtml));
    if (string != NULL) {
      gtk_imhtml_append_text (GTK_IMHTML(imhtml), string,
                     GTK_IMHTML_NO_SCROLL); 
      gtk_imhtml_append_text (GTK_IMHTML(imhtml), "<BR>", 
                     GTK_IMHTML_NO_SCROLL);
    }
  }
}

/*--------------------------------------------------------------------------*
 * Progress bar stuff                                                       *
 *--------------------------------------------------------------------------*/
static APProgressBar *progress_create (APUpdateType type, 
                GtkWidget *container)
{
  APProgressBar *progress_bar;

  progress_bar = (APProgressBar *) malloc (sizeof (APProgressBar));
  progress_bar->timeout = 0;
  progress_bar->type = type;
  progress_bar->bar = gtk_progress_bar_new ();
  gtk_progress_bar_set_bar_style (GTK_PROGRESS_BAR(progress_bar->bar), GTK_PROGRESS_CONTINUOUS);
  gtk_box_pack_start (GTK_BOX(container), progress_bar->bar, FALSE, FALSE, 0);
  if (type == AP_UPDATE_PROFILE) {
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress_bar->bar), 
                  _("no updates made to profile"));
  } else if (type == AP_UPDATE_STATUS) {
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress_bar->bar), 
                  _("no updates made to status"));
  }
  
  g_hash_table_insert (progress_bars, GINT_TO_POINTER(type), progress_bar);

  return progress_bar;
}

static void progress_update_stop (APProgressBar *progress_bar) {
  if (progress_bar->timeout) {
    purple_timeout_remove (progress_bar->timeout);
    progress_bar->timeout = 0;
  }
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress_bar->bar), 1.0);
  if (progress_bar->type == AP_UPDATE_PROFILE) {
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress_bar->bar), 
                  _("waiting for new profile content"));
  } else if (progress_bar->type == AP_UPDATE_STATUS) {
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress_bar->bar), 
                  _("waiting for new status content"));
  }
}

#define BAH 500

static gboolean progress_update (gpointer data) {
  int total_milliseconds;
  int seconds_remaining;
  double fraction_increment;
  double cur_fraction;
  double result;
  GString *text;
  APProgressBar *progress_bar = (APProgressBar *) data;
  
  // Update fraction on bar
  total_milliseconds = 
          purple_prefs_get_int ("/plugins/gtk/autoprofile/delay_update") * 1000;
  fraction_increment = BAH / ((double) total_milliseconds);
  cur_fraction = gtk_progress_bar_get_fraction (
           GTK_PROGRESS_BAR(progress_bar->bar));
  result = cur_fraction + fraction_increment;
  if (result >= 1) {
    progress_update_stop (progress_bar);
    return FALSE;
  }
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress_bar->bar), result);

  // Update text on bar
  seconds_remaining = (int) 
          (((double) total_milliseconds / 1000) -
           (cur_fraction * (double) total_milliseconds / 1000));
  text = g_string_new ("");
  if (progress_bar->type == AP_UPDATE_PROFILE) {
    g_string_printf (text, _("next profile update in %d seconds"), 
                     seconds_remaining);
  } else if (progress_bar->type == AP_UPDATE_STATUS) {
    g_string_printf (text, _("next status update in %d seconds"), 
                     seconds_remaining);
  }
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress_bar->bar), text->str);
  g_string_free (text, TRUE);

  return TRUE;
}

static void ap_gtk_timeout_start (APUpdateType type) {
  APProgressBar *progress_bar;

  progress_bar = g_hash_table_lookup (progress_bars, GINT_TO_POINTER(type));
  if (progress_bar->timeout) {
    purple_timeout_remove (progress_bar->timeout);
  }
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress_bar->bar), 0);
  progress_bar->timeout = 
          purple_timeout_add (BAH, progress_update, progress_bar);
  progress_update (progress_bar);
}

void ap_gtk_set_progress_visible (APUpdateType type, gboolean visible)
{
  APProgressBar *progress_bar;

  progress_bar = g_hash_table_lookup (progress_bars, GINT_TO_POINTER(type));
  if (visible) gtk_widget_show (progress_bar->bar);
  else         gtk_widget_hide (progress_bar->bar);
}

/*--------------------------------------------------------------------------*
 * Create the main window                                                   *
 *--------------------------------------------------------------------------*/
static void create_dialog () {
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeSelection *selection;
  GtkWidget *message_list_view;
  
  GtkWidget *vbox, *vpane, *hbox, *config_vbox;
  GtkWidget *sw, *imhtml, *msg_window, *button;

  imhtml = gtk_imhtml_new (NULL, NULL);
  
  /* Create main display window */
  PIDGIN_DIALOG(dialog);
  gtk_window_set_title (GTK_WINDOW(dialog), _("AutoProfile Summary"));
  gtk_widget_realize (dialog);

  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_add (GTK_CONTAINER (dialog), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  /* Set up progress bar container */
  progress_create (AP_UPDATE_PROFILE, vbox);
  progress_create (AP_UPDATE_STATUS, vbox);

  /* Set up list of past away messages */
  vpane = gtk_vpaned_new ();
  gtk_box_pack_start (GTK_BOX(vbox), vpane, TRUE, TRUE, 0);
  
  message_list = gtk_list_store_new (4, 
    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  message_list_view = gtk_tree_view_new_with_model (
    GTK_TREE_MODEL (message_list));
  renderer = gtk_cell_renderer_text_new ();

  column = gtk_tree_view_column_new_with_attributes (
    _("Time"), renderer, "markup", 0, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (message_list_view), column);
  gtk_tree_view_column_set_sort_column_id (column, 0);

  column = gtk_tree_view_column_new_with_attributes (
    _("Type"), renderer, "markup", 1, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (message_list_view), column);
  gtk_tree_view_column_set_sort_column_id (column, 1);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

  column = gtk_tree_view_column_new_with_attributes (
    _("Text"), renderer, "markup", 2, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (message_list_view), column);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(sw), 
    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(sw), 
    GTK_SHADOW_IN);

  gtk_container_add (GTK_CONTAINER (sw), message_list_view);
  gtk_paned_add1 (GTK_PANED(vpane), sw);
 
  selection = gtk_tree_view_get_selection (
    GTK_TREE_VIEW (message_list_view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (display_diff_msg), imhtml);

  /* Set up the window to display away message in */
  msg_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(msg_window), 
    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(msg_window), 
    GTK_SHADOW_IN);
  gtk_paned_add2 (GTK_PANED(vpane), msg_window);
 
  gtk_container_add (GTK_CONTAINER(msg_window), imhtml);
  pidgin_setup_imhtml (imhtml);
 
  /* Bottom area */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  config_vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX(hbox), config_vbox, TRUE, TRUE, 0);
 
  pidgin_prefs_checkbox (
    _("Queue new messages while away"),
    "/plugins/gtk/autoprofile/queue_messages_when_away", 
    config_vbox);

  button = pidgin_prefs_checkbox (
    _("Play sounds while away"),
    "/core/sound/while_away",
    config_vbox);
  sound_pref_cb = purple_prefs_connect_callback (ap_get_plugin_handle (), 
    "/core/sound/while_away", sound_cb, button);

  gtk_box_pack_start (GTK_BOX(hbox), gtk_vseparator_new (), FALSE, FALSE, 0);

  config_vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX(hbox), config_vbox, TRUE, TRUE, 0);
  
  ap_gtk_prefs_add_summary_option (config_vbox);

  button = gtk_button_new_with_label (_("Hide summary now"));  
  gtk_box_pack_start (GTK_BOX(config_vbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (hide_cb), NULL);

  /* Finish up */
  g_signal_connect (G_OBJECT(dialog), "delete-event",
    G_CALLBACK(gtk_widget_hide_on_delete), NULL);
  gtk_paned_set_position (GTK_PANED(vpane), 250);
  gtk_window_set_default_size (GTK_WINDOW(dialog), 430, 430);
}

/*--------------------------------------------------------------------------*
 * PUBLIC FUNCTIONS                                                         *
 *--------------------------------------------------------------------------*/
void ap_gtk_add_message (APUpdateType update_type, APMessageType type, 
                const gchar *text) 
{
  GtkTreeIter iter;
  struct tm *cur_time;
  char *time_string, *simple_text, *s;
  time_t *general_time;
  gchar *type_string;
  
  // Create the time string 
  general_time = (time_t *) malloc (sizeof(time_t));
  time (general_time);
  cur_time = ap_localtime (general_time);
  free (general_time);

  time_string = (char *) malloc (sizeof(char[32]));
  *time_string = '\0';

  strftime (time_string, 31, "<b>%I:%M %p</b>", cur_time);
  free (cur_time);
 
  // Create the type string
  type_string = strdup("<b>Status</b>");
  switch (type) {
    case AP_MESSAGE_TYPE_PROFILE:
      type_string = strdup (_("<b>User profile</b>"));
      break;
    case AP_MESSAGE_TYPE_AWAY:
      type_string = strdup (_("<b>Away message</b>"));
      break;
    case AP_MESSAGE_TYPE_AVAILABLE:
      type_string = strdup (_("<b>Available message</b>"));
      break;
    case AP_MESSAGE_TYPE_STATUS:
      type_string = strdup (_("<b>Status message</b>"));
      break;
    default:
      type_string = strdup (_("<b>Other</b>"));
      break;
  }

  // Simplify the text
  if (text != NULL) {
    simple_text = strdup (text);

    // Only show the first line 
    s = (gchar *) purple_strcasestr (simple_text, "<br>");
    if (s != NULL) {
      *s++ = '.';
      *s++ = '.';
      *s++ = '.';
      *s   = '\0';
    }

    // Strip HTML
    s = simple_text;
    simple_text = purple_markup_strip_html (simple_text);
    free (s);
    
  } else {
    simple_text = NULL;
  }  

  // Add it 
  gtk_list_store_prepend (message_list, &iter);
  gtk_list_store_set (message_list, &iter,
                      0, time_string,
                      1, type_string,
                      2, simple_text,
                      3, text,
                      -1);
  free (type_string);
  free (time_string);
  if (simple_text) free (simple_text);

  // Delete if too many
  if (gtk_tree_model_iter_nth_child 
        (GTK_TREE_MODEL(message_list), &iter, NULL, 
         AP_GTK_MAX_MESSAGES)) {
    gtk_list_store_remove (message_list, &iter);
  }

  // Move the timeout bar
  ap_gtk_timeout_start (update_type);

  // Check if it needs to be visible or not
  if (type != AP_MESSAGE_TYPE_PROFILE && 
      ap_is_currently_away () != ap_previously_away) {
    update_summary_visibility ();
  }
}

void ap_gtk_make_visible ()
{
  gtk_widget_show_all (dialog);
  gtk_window_present (GTK_WINDOW(dialog));
}

void ap_gtk_start () {
  progress_bars = g_hash_table_new (NULL, NULL);
        
  // Message queueing 
  queue_pref_cb = purple_prefs_connect_callback (
    ap_get_plugin_handle (), 
    "/plugins/gtk/autoprofile/queue_messages_when_away", queue_cb, NULL);

  // Create window
  create_dialog ();
  update_summary_visibility ();
}

static void ap_gtk_finish_progress_bar (APUpdateType type) 
{
  APProgressBar *progress_bar;

  progress_bar = g_hash_table_lookup (progress_bars, GINT_TO_POINTER(type));
  if (progress_bar) {
    if (progress_bar->timeout) {
      purple_timeout_remove (progress_bar->timeout);
    }
    free (progress_bar);
    g_hash_table_insert (progress_bars, GINT_TO_POINTER(type), NULL);
  }
}

void ap_gtk_finish () {
  // Kill the window and associated variables
  gtk_widget_destroy (dialog);
  dialog = NULL;
  message_list = NULL;

  ap_gtk_finish_progress_bar (AP_UPDATE_PROFILE);
  ap_gtk_finish_progress_bar (AP_UPDATE_STATUS);

  // Disconnect queue message
  purple_prefs_disconnect_callback (queue_pref_cb);
  queue_pref_cb = 0;
  purple_prefs_disconnect_callback (sound_pref_cb);
  sound_pref_cb = 0;

  g_hash_table_destroy (progress_bars);
  progress_bars = NULL;
}


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
#include "comp_logstats.h"
#include "request.h"

GtkWidget *checkbox = NULL;

GtkListStore *alias_list = NULL;
GtkWidget *alias_view = NULL;

/* General callbacks from main preferences */
static void logstats_response_cb (GtkDialog *dialog, gint id, 
  GtkWidget *widget) 
{
  purple_prefs_set_bool (
    "/plugins/gtk/autoprofile/components/logstat/enabled", TRUE);
  logstats_load ();
  gtk_widget_set_sensitive (widget, TRUE);

  gtk_widget_destroy (GTK_WIDGET(dialog));
}

static void toggle_enable (GtkButton *button, gpointer data)
{
  GtkWidget *popup, *vbox, *label;
  vbox = data;

  if (purple_prefs_get_bool (
    "/plugins/gtk/autoprofile/components/logstat/enabled")) {
    logstats_unload ();
    purple_prefs_set_bool (
      "/plugins/gtk/autoprofile/components/logstat/enabled", FALSE);
    gtk_widget_set_sensitive (vbox, FALSE);
  } else {
    popup = gtk_dialog_new_with_buttons (
      "Enable stats for logs", NULL, 0,
      GTK_STOCK_OK, 42, NULL);
    g_signal_connect (G_OBJECT(popup), "response",
      G_CALLBACK(logstats_response_cb), vbox);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), 
      "\nEnabling this component will have some minor side effects.  Doing so "
      "will cause Purple to take slightly longer to start up because it must "
      "parse a large amount of data to gather statistics.  On average, this "
      "can take slightly over a second for every 100,000 messages in your "
      "logs.\n\nThe time from when you press the OK button to the time "
      "when this dialog vanishes is a good approximation of how much extra "
      "time will elapse before the login screen is shown.\n"
    );
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(popup)->vbox), label, 
      FALSE, FALSE, 0);

    gtk_widget_show_all (popup);
  }
}

static gboolean logstat_format (GtkWidget *widget, GdkEventFocus *event,
  gpointer data)
{
  purple_prefs_set_string (
    "/plugins/gtk/autoprofile/components/logstat/format",
    gtk_entry_get_text (GTK_ENTRY (widget)));
  return FALSE;
}

static void new_alias (gpointer data, PurpleRequestFields *fields)
{
  GtkTreeIter iter;
  GList *aliases;

  const char *alias;

  alias = purple_request_fields_get_string (fields, "alias");
  aliases = purple_prefs_get_string_list (
    "/plugins/gtk/autoprofile/components/logstat/aliases");

  aliases = g_list_append (aliases, strdup (alias));
  purple_prefs_set_string_list (
    "/plugins/gtk/autoprofile/components/logstat/aliases", aliases);
  free_string_list (aliases);

  gtk_list_store_insert (alias_list, &iter, 0);
  gtk_list_store_set (alias_list, &iter,
                      0, alias, -1);
}

static void alias_add (GtkButton *button, gpointer data)
{
  PurpleRequestFields *fields;
  PurpleRequestFieldGroup *group;
  PurpleRequestField *field;

  fields = purple_request_fields_new();

  group = purple_request_field_group_new(NULL);
  purple_request_fields_add_group(fields, group);

  field = purple_request_field_string_new("alias", _("Alias"),
    NULL, FALSE);
  purple_request_field_set_required(field, TRUE);
  purple_request_field_set_type_hint(field, "alias");
  purple_request_field_group_add_field(group, field);

  purple_request_fields(purple_get_blist(), _("Add Alias"),
    NULL,
    _("Type in the alias that you use"),
    fields,
    _("OK"), G_CALLBACK(new_alias),
    _("Cancel"), NULL,
    NULL);
}

static void alias_delete (GtkButton *button, gpointer data)
{
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  char *alias;

  GList *aliases, *aliases_start, *new_aliases;

  selection = gtk_tree_view_get_selection (
    GTK_TREE_VIEW (alias_view));

  if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    return;

  gtk_tree_model_get (GTK_TREE_MODEL (alias_list), &iter,
                      0, &alias, -1);
  gtk_list_store_remove (alias_list, &iter);

  aliases = purple_prefs_get_string_list (
    "/plugins/gtk/autoprofile/components/logstat/aliases");
  aliases_start = aliases;

  new_aliases = NULL;

  while (aliases) {
    if (strcmp ((char *)aliases->data, alias)) {
      new_aliases = g_list_append (new_aliases, aliases->data);
    }

    aliases = aliases->next;
  }

  purple_prefs_set_string_list (
    "/plugins/gtk/autoprofile/components/logstat/aliases", new_aliases);

  free_string_list (aliases_start);
  g_list_free (new_aliases);
  free (alias);
}

static void alias_what (GtkButton *button, gpointer data)
{
  purple_notify_formatted (NULL, _("Aliases"), _("What this list is for"), NULL,
    _("Logs in Purple are stored verbatim with what you see on the screen.  "
      "The names of the people in the conversation (both yourself and your "
      "buddy) are shown with their given aliases as opposed to actual screen "
      "names.  If you have given yourself an alias in a conversation, list "
      "it using this dialog.  If you do not, messages written by you will "
      "be incorrectly identified as received instead of sent.<br><br>Correct "
      "capitalization and whitespace are not required for detection to "
      "work.<br><br>You must disable/re-enable log stats to refresh the "
      "database after an alias change."), 
      NULL, NULL);
}

/* The main window */
GtkWidget *logstats_prefs (void) 
{
  GtkWidget *ret, *vbox, *hbox;
  GtkWidget *label, *button, *entry, *sw;
  
  GtkCellRenderer *renderer;
  GtkTreeSelection *selection;
  GtkTreeViewColumn *col;
  GtkTreeIter iter;
  GList *aliases, *aliases_start;

  ret = gtk_vbox_new (FALSE, 6);

  /* Checkbox for enabling/disabling */
  checkbox = gtk_check_button_new_with_mnemonic (
    "Enable statistics for logs");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(checkbox),
    purple_prefs_get_bool (
      "/plugins/gtk/autoprofile/components/logstat/enabled"));
  gtk_box_pack_start (GTK_BOX(ret), checkbox, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX(ret), vbox, TRUE, TRUE, 0);

  /* Le format string */
  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL(label), "<b>Format string for output</b>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_box_pack_start (GTK_BOX(vbox), label, FALSE, FALSE, 0);

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX(vbox), entry, FALSE, FALSE, 0);
  gtk_entry_set_max_length (GTK_ENTRY(entry), 1000);
  gtk_entry_set_text (GTK_ENTRY(entry), purple_prefs_get_string (
    "/plugins/gtk/autoprofile/components/logstat/format"));
  g_signal_connect (G_OBJECT (entry), "focus-out-event",
                    G_CALLBACK (logstat_format), NULL);

  label = gtk_label_new (_(
    "%R\tTotal messages received\n"
    "%r\tTotal words received\n"
    "%S\tTotal messages sent\n"
    "%s\tTotal words sent\n"
    "%T\tTotal messages sent/received\n"
    "%t\tTotal words sent/received\n"
    "%D\tNumber of days since first logged conversation\n"
    "%d\tNumber of days with logged conversations\n"
    "%N\tNumber of logged conversations\n"
    "%n\tAverage number of conversations per day with logs\n"
    "%i\tMost conversations in a single day\n"
    "%I\tDate with most conversations\n"
    "%j\tMost messages sent in a single day\n"
    "%J\tDate with most messages sent\n"
    "%k\tMost messages received in a single day\n"
    "%K\tDate with most messages received\n"
    "%l\tMost total messages sent/received in a single day\n"
    "%L\tDate with most total messages sent/received\n"
    "%f\tDate of first logged conversation\n"
    "%u\tAverage words per message received\n"
    "%v\tAverage words per message sent\n"
    "%w\tAverage words per message sent/received\n"
    "%U\tAverage messages received per conversation\n"
    "%V\tAverage messages sent per conversation\n"
    "%W\tAverage messages sent/received per conversation\n"
    "%x\tAverage words received per day with logs\n"
    "%y\tAverage words sent per day with logs\n"
    "%z\tAverage words sent/received per day with logs\n"
    "%X\tAverage messages received per day with logs\n"
    "%Y\tAverage messages sent per day with logs\n"
    "%Z\tAverage messages sent/received per day with logs\n"
    "%p\tPercentage of days with logs\n"
    "%a\tNumber of messages received today\n"
    "%b\tNumber of messages sent today\n"
    "%c\tNumber of conversations started today\n"
    "%e\tNumber of messages sent/received today\n"
    "%A\tNumber of messages received in last week\n"
    "%B\tNumber of messages sent in last week\n"
    "%C\tNumber of conversations started in last week\n"
    "%E\tNumber of messages sent/received in last week\n"
    "%%\t%"));

  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  sw = gtk_scrolled_window_new (NULL,NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE , 0);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(sw), label);

  /* Aliases */
  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL(label), "<b>Personal aliases</b>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_box_pack_start (GTK_BOX(vbox), label, FALSE, FALSE, 0);

  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL(label), 
    "You need this if you have an alias for your own screen name,\n"
    "else IM's you sent will be incorrectly counted as received");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_box_pack_start (GTK_BOX(vbox), label, FALSE, FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_label (_("Add alias"));
  g_signal_connect (G_OBJECT(button), "clicked",
    G_CALLBACK (alias_add), NULL);
  gtk_box_pack_start (GTK_BOX(hbox), button, TRUE, TRUE, 0); 
  button = gtk_button_new_with_label (_("Delete alias"));
  g_signal_connect (G_OBJECT(button), "clicked",
    G_CALLBACK (alias_delete), NULL);
  gtk_box_pack_start (GTK_BOX(hbox), button, TRUE, TRUE, 0); 
  button = gtk_button_new_with_label (_("?"));
  g_signal_connect (G_OBJECT(button), "clicked",
    G_CALLBACK (alias_what), NULL);
  gtk_box_pack_start (GTK_BOX(hbox), button, FALSE, FALSE, 0); 

  sw = gtk_scrolled_window_new (0, 0);
  gtk_box_pack_start (GTK_BOX(vbox), sw, FALSE, FALSE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(sw),
    GTK_POLICY_NEVER, GTK_POLICY_NEVER);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(sw),
    GTK_SHADOW_IN);

  alias_list = gtk_list_store_new (1, G_TYPE_STRING);  
  alias_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (alias_list));
  gtk_container_add (GTK_CONTAINER(sw), alias_view);
 
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(alias_view), FALSE);
  selection = gtk_tree_view_get_selection (
    GTK_TREE_VIEW (alias_view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

  col = gtk_tree_view_column_new_with_attributes (
    _("Alias"), renderer, "text", 0, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(alias_view), col);

  aliases = purple_prefs_get_string_list (
    "/plugins/gtk/autoprofile/components/logstat/aliases");
  aliases_start = aliases;

  while (aliases) {
    gtk_list_store_append (alias_list, &iter);
    gtk_list_store_set (alias_list, &iter,
      0, (char *)aliases->data, -1);
    aliases = aliases->next;
  }
  free_string_list (aliases_start);

  /* Finish up the checkbox stuff */
  g_signal_connect (G_OBJECT(checkbox), "clicked",
    G_CALLBACK(toggle_enable), vbox);
  if (!purple_prefs_get_bool (
    "/plugins/gtk/autoprofile/components/logstat/enabled")) {
    gtk_widget_set_sensitive (vbox, FALSE);
  } else {
    gtk_widget_set_sensitive (vbox, TRUE);
  }

  return ret;
}

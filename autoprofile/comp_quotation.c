/*----------------------------------------------------------------------------*
 * AUTOPROFILE                                                                *
 *                                                                            *
 * A Purple away message and profile manager that supports dynamic text         *
 *                                                                            *
 * AutoProfile is the legal property of its developers.  Please refer to      *
 * the COPYRIGHT file distributed with this source distribution.              *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation; either version 2 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             * 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          * 
 * along with this program; if not, write to the Free Software                *
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA   *
 *----------------------------------------------------------------------------*/

#include "autoprofile.h"
#include "request.h"

#include "component.h"
#include "utility.h"

enum {
  QUOTATION_LIST_STORE = 1,
  QUOTATION_FILE_SELECTOR,
  QUOTATION_TREE_VIEW
};

/*--------------------------------------------------------------------------*
 * Menu related things                                                      *
 *--------------------------------------------------------------------------*/
static void append_quote (struct widget *w, GtkListStore *ls, gchar *quote)
{
  GString *s;
  GtkTreeIter iter;
  gchar *quote_tmp;
  GtkWidget *treeview;
  GtkTreeSelection *selection;
  
  gtk_list_store_append (ls, &iter);

  quote_tmp = purple_markup_strip_html (quote); 
  s = g_string_new ("");
  g_string_printf (s, "%d bytes", strlen (quote));

  gtk_list_store_set (ls, &iter, 
                      0, quote_tmp, 
                      1, quote, 
                      2, s->str,
                      -1);
  g_free (quote_tmp);
  g_string_free (s, TRUE);

  treeview = (GtkWidget *) ap_widget_get_data (w, QUOTATION_TREE_VIEW);
  if (treeview == NULL) return;
  
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(treeview));
  gtk_tree_selection_select_iter (selection, &iter); 
}

static void file_dialog_cb (GtkWidget *dialog, int response, struct widget *w)
{
  GtkWidget *checkbox;
  gchar *filename;
  GList *quotes, *quotes_start, *new_quotes;
  gboolean include_html;

  GtkListStore *ls;

  switch (response) {
    case GTK_RESPONSE_ACCEPT:
      ls = ap_widget_get_data (w, QUOTATION_LIST_STORE);
      if (ls == NULL) break;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(dialog));
      checkbox = gtk_file_chooser_get_extra_widget (GTK_FILE_CHOOSER(dialog));
      g_object_get (checkbox, "active", &include_html, NULL);

      quotes = ap_prefs_get_string_list (w, "quotes");
      new_quotes = read_fortune_file (filename, !include_html);
      g_free (filename);
      
      quotes = g_list_concat (quotes, new_quotes);
      ap_prefs_set_string_list (w, "quotes", quotes);
      
      quotes_start = quotes;
      
      for (quotes = new_quotes; quotes != NULL; quotes = quotes->next) {
        append_quote (w, ls, quotes->data);
      }

      free_string_list (quotes_start);
      break;
    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_DELETE_EVENT:
      break;
  }

  ap_widget_set_data (w, QUOTATION_FILE_SELECTOR, NULL);
  gtk_widget_destroy (dialog);
}

static void quotation_explain_fortune_file (GtkMenuItem *item, gpointer data)
{
  purple_notify_formatted (NULL, _("Fortune files"), 
    _("A quick definition of a fortune file"), NULL,
    _("A fortune file is a simple text file with a number of quotes. "
      "The following is an example:<br><br>"
      "<b>\"Glory is fleeing, but obscurity is forver.\"<br>"
      "- Napoleon Bonaparte (1769-1821)<br>"
      "%<br>"
      "Blagggghhhh!<br>"
      "%<br>"
      "Yet another quote<br>"
      "%<br></b><br>"
      "Quotes can have any sort of text within them.  They end when there "
      "is a newline followed by a percent sign \"%\" on the next line.<br>"
      "<br>Fortune files with pre-selected quotes can be found on the"
      "internet."),
    NULL, NULL);
}

static void quotation_select_import_file (GtkMenuItem *item, struct widget *w)
{
  GtkWidget *dialog;
  GtkWidget *checkbox;

  dialog = gtk_file_chooser_dialog_new (
    _("Select fortune file to import quotes from"),
    NULL,
    GTK_FILE_CHOOSER_ACTION_OPEN,
    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
    NULL);
  g_signal_connect (G_OBJECT(dialog), "response", G_CALLBACK (file_dialog_cb),
    w);
  ap_widget_set_data (w, QUOTATION_FILE_SELECTOR, dialog);
  
  checkbox = gtk_check_button_new_with_label (
    _("Interpret bracketed text (such as \"<br>\") as HTML tags"));
  gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER(dialog), checkbox);

  gtk_widget_show_all (dialog);
}

static void quotation_edit_dialog_cb (struct widget *w, const char *quote)
{
  GtkWidget *treeview;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  GtkTreeModel *model;
  
  treeview = (GtkWidget *) ap_widget_get_data (w, QUOTATION_TREE_VIEW);
  if (treeview == NULL) return;
  
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(treeview));

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    GString *s;
    gchar *quote_tmp, *old_quote;
    GList *start, *node;
    
    gtk_tree_model_get (model, &iter, 1, &old_quote, -1);
    start = ap_prefs_get_string_list (w, "quotes"); 
    /* FIXME: this could grab the wrong quote, if two quotes are identical */
    for (node = start; node != NULL; node = node->next) {
      if (!strcmp ((char *) node->data, old_quote)) {
        /* Update saved prefs */
        g_free (node->data);
        node->data = strdup (quote);
              
        ap_prefs_set_string_list (w, "quotes", start);

        free_string_list (start);
        g_free (old_quote);
       
        /* Update list store */ 
        quote_tmp = purple_markup_strip_html (quote); 
        s = g_string_new ("");
        g_string_printf (s, "%d bytes", strlen (quote));

        gtk_list_store_set (GTK_LIST_STORE (model), &iter, 
                            0, quote_tmp, 
                            1, quote, 
                            2, s->str,
                            -1);
        g_free (quote_tmp);
        g_string_free (s, TRUE);
        return;
      }
    }

    free_string_list (start);
    g_free (old_quote);
  } else {
    purple_notify_error (NULL, NULL,
      N_("Unable to edit quote"),
      N_("No quote is currently selected"));
  }

}

static void quotation_edit_dialog (struct widget *w, const gchar *quote) 
{
  purple_request_input (ap_get_plugin_handle (), NULL,
                      _("Edit quote"), NULL, 
                      quote,
                      TRUE, FALSE, "html",
                      _("Save"), G_CALLBACK(quotation_edit_dialog_cb),
                      _("Cancel"), NULL,
                      w);
}

static void quotation_edit (GtkWidget *button, struct widget *w)
{
  GtkWidget *treeview;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *quote;
  
  treeview = (GtkWidget *) ap_widget_get_data (w, QUOTATION_TREE_VIEW);
  if (treeview == NULL) return;
  
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(treeview));

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get (model, &iter, 1, &quote, -1);
    quotation_edit_dialog (w, quote);
    g_free (quote);
  } else {
    purple_notify_error (NULL, NULL,
      N_("Unable to edit quote"),
      N_("No quote is currently selected"));
  }
}

static void quotation_create (GtkWidget *button, struct widget *w)
{
  GtkListStore *ls;
  GList *quotes;
  
  ls = ap_widget_get_data (w, QUOTATION_LIST_STORE);
  if (ls == NULL) return;
  
  append_quote (w, ls, "");

  quotes = ap_prefs_get_string_list (w, "quotes");
  quotes = g_list_append (quotes, strdup (""));
  ap_prefs_set_string_list (w, "quotes", quotes);

  free_string_list (quotes);

  quotation_edit_dialog (w, "");
}

static void quotation_delete (GtkWidget *button, struct widget *w)
{
  GtkWidget *treeview;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *quote;
  
  GList *start, *node;
  
  treeview = (GtkWidget *) ap_widget_get_data (w, QUOTATION_TREE_VIEW);
  if (treeview == NULL) return;
  
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(treeview));

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get (model, &iter, 1, &quote, -1);
    start = ap_prefs_get_string_list (w, "quotes");

    /* FIXME: this could grab the wrong quote, if two quotes are identical */
    for (node = start; node != NULL; node = node->next) {
      if (!strcmp ((char *) node->data, quote)) {
        start = g_list_remove_link (start, node);
        g_list_free_1 (node);
        g_free (node->data);
        ap_prefs_set_string_list (w, "quotes", start);

        free_string_list (start);
        g_free (quote);

        gtk_list_store_remove (GTK_LIST_STORE(model), &iter);
        return;
      }
    }

    free_string_list (start);
    g_free (quote);
  } else {
    purple_notify_error (NULL, NULL,
      N_("Unable to delete quote"),
      N_("No quote is currently selected"));
  }
}

static void quotation_delete_all_cb (struct widget *w)
{
  GtkListStore *ls;
  
  ls = ap_widget_get_data (w, QUOTATION_LIST_STORE);
  if (ls == NULL) return;
  
  gtk_list_store_clear (ls);

  ap_prefs_set_string_list (w, "quotes", NULL);
}
  
static void quotation_delete_all (GtkMenuItem *item, struct widget *w)
{
  purple_request_ok_cancel (ap_get_plugin_handle (),
                          NULL, _("Delete all quotes?"), NULL, 0,
                          w, G_CALLBACK(quotation_delete_all_cb), NULL);
}

static void quotation_more_menu (GtkWidget *button, struct widget *w)
{
  GtkWidget *menu;
  GtkWidget *menu_item;

  menu = gtk_menu_new ();

  menu_item = gtk_menu_item_new_with_label (_("Delete all quotes"));
  gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item);
  g_signal_connect (G_OBJECT(menu_item), "activate",
    G_CALLBACK(quotation_delete_all), w);  

  menu_item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item);
  
  menu_item = gtk_menu_item_new_with_label (
                  _("Import quotes from from fortune file"));
  gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item);
  g_signal_connect (G_OBJECT(menu_item), "activate",
    G_CALLBACK(quotation_select_import_file), w);  

  menu_item = gtk_menu_item_new_with_label (
                  _("What is a fortune file?"));
  gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item); 
  g_signal_connect (G_OBJECT(menu_item), "activate",
    G_CALLBACK(quotation_explain_fortune_file), NULL);

  gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, w, 0, 
                  gtk_get_current_event_time ());
  gtk_widget_show_all (menu);
}

static void quotation_rate_changed (GtkSpinButton *spinner, struct widget *w)
{
  int value = gtk_spin_button_get_value_as_int (spinner);
  ap_prefs_set_int (w, "update_rate", value);
}

static void quotation_force_change (GtkButton *button, struct widget *w)
{
  ap_prefs_set_int (w, "current_index",
                    ap_prefs_get_int (w, "current_index") + 1);
}



static gboolean
search_func(GtkTreeModel *model, gint column, const gchar *key, 
  GtkTreeIter *iter, gpointer search_data)
{
  gboolean result;
  char *haystack;

  gtk_tree_model_get (model, iter, 1, &haystack, -1);
  result = (purple_strcasestr(haystack, key) == NULL);
  g_free(haystack);

  return result;
}

static void menu_destroy_cb (GtkWidget *widget, struct widget *w)
{
  GtkWidget *file_selector;
        
  ap_widget_set_data (w, QUOTATION_LIST_STORE, NULL);
  ap_widget_set_data (w, QUOTATION_TREE_VIEW, NULL);
  
  file_selector = (GtkWidget *) ap_widget_get_data (w, QUOTATION_FILE_SELECTOR);
  if (file_selector != NULL) {
    file_dialog_cb (file_selector, GTK_RESPONSE_DELETE_EVENT, w);
  }
}

static GtkWidget *quotation_menu (struct widget *w)
{
  GtkWidget *ret, *hbox;
  GtkWidget *button, *label, *spinner;
  GtkWidget *sw;
  GList *quotes, *quotes_start;

  GtkWidget *treeview;
  GtkListStore *ls;
  GtkTreeViewColumn *col;
  GtkCellRenderer *rend; 

  ret = gtk_vbox_new (FALSE, 6);
  g_signal_connect (G_OBJECT(ret), "destroy", G_CALLBACK (menu_destroy_cb), w);
  
  /* The main view */
  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(sw),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(sw), 
    GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX(ret), sw, TRUE, TRUE, 0);

  ls = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  ap_widget_set_data (w, QUOTATION_LIST_STORE, ls);

  treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL(ls));
  ap_widget_set_data (w, QUOTATION_TREE_VIEW, treeview);
  
  rend = gtk_cell_renderer_text_new ();
  col = gtk_tree_view_column_new_with_attributes (_("Size"),
    rend, "text", 2, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), col);
  g_object_set (G_OBJECT(rend),
                "cell-background-set", TRUE,
                "cell-background", "gray",
                NULL);

  rend = gtk_cell_renderer_text_new ();
  col = gtk_tree_view_column_new_with_attributes (_("Quotes"),
    rend, "text", 0, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), col);

  /* Enable CTRL+F searching */
  gtk_tree_view_set_search_column (GTK_TREE_VIEW(treeview), 0);
  gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW(treeview), 
    search_func, NULL, NULL);

  gtk_container_add (GTK_CONTAINER(sw), treeview);

  /* Add in the original quotes */
  quotes_start = ap_prefs_get_string_list (w, "quotes");

  for (quotes = quotes_start; quotes != NULL; quotes = quotes->next) {
    append_quote (w, ls, quotes->data);
  }
  free_string_list (quotes_start);

  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW(treeview), TRUE);

  /* Bottom buttons */
  hbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX(hbox), 
                             GTK_BUTTONBOX_SPREAD);

  gtk_box_pack_start (GTK_BOX(ret), hbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_label (_("New quote"));
  gtk_box_pack_start (GTK_BOX(hbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT(button), "clicked",
                    G_CALLBACK(quotation_create), w);
  
  button = gtk_button_new_with_label (_("Edit"));
  gtk_box_pack_start (GTK_BOX(hbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT(button), "clicked",
                    G_CALLBACK(quotation_edit), w);
  
  button = gtk_button_new_with_label (_("Delete"));
  gtk_box_pack_start (GTK_BOX(hbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT(button), "clicked",
                    G_CALLBACK(quotation_delete), w);
  
  button = gtk_button_new_with_label (_("More..."));
  gtk_box_pack_start (GTK_BOX(hbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT(button), "clicked",
                    G_CALLBACK(quotation_more_menu), w);

  /* Separator */
  gtk_box_pack_start (GTK_BOX(ret), gtk_hseparator_new (), FALSE, FALSE, 0);
  
  /* Behavior selection */
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX(ret), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Change quote every "));
  gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 0);

  spinner = gtk_spin_button_new_with_range (0, G_MAXINT, 1);
  gtk_box_pack_start (GTK_BOX(hbox), spinner, FALSE, FALSE, 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinner),
                  ap_prefs_get_int (w, "update_rate"));
  g_signal_connect (G_OBJECT(spinner), "value-changed",
                    G_CALLBACK(quotation_rate_changed), w);
  
  label = gtk_label_new (_("hours (0: always show a new quote)"));
  gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 0);

  button = gtk_button_new_with_label (_("Change quote now"));
  gtk_box_pack_end (GTK_BOX(hbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (quotation_force_change), w);
  
  return ret;
}


/*--------------------------------------------------------------------------*
 * Core quotation things                                                    *
 *--------------------------------------------------------------------------*/
static gchar *quotation_generate (struct widget *w)
{
  GList *quotes;
  gchar *ret;
  int num_quotes, index;
  time_t cur_time, old_time;
  char *time_string;
  struct tm *t;

  index = ap_prefs_get_int (w, "current_index");
  quotes = ap_prefs_get_string_list (w, "quotes");

  /* Sanity check the quotes */
  num_quotes = g_list_length (quotes);
  
  if (num_quotes == 0) {
    return strdup (_("[ERROR: no quotes available]"));
  }  

  /* Increment index if time has elapsed */
  old_time = purple_str_to_time (ap_prefs_get_string (w, "last_update"), TRUE,
                   NULL, NULL, NULL);
  cur_time = time (NULL);

  if (difftime (cur_time, old_time) > 
    60.0 * 60.0 * (double) ap_prefs_get_int (w, "update_rate")) 
  {
    ap_debug ("quote", "time interval elapsed, moving to new quote");
      
    time_string = (char *)malloc(1000);
    t = ap_gmtime (&cur_time);
    strftime (time_string, 999, "%Y-%m-%dT%H:%M:%S+00:00", t);
    free (t);
    ap_prefs_set_string (w, "last_update", time_string);
    free (time_string);

    index++;
    ap_prefs_set_int (w, "current_index", index);
  }

  /* Wrap around when last quote is reached */
  if (index >= num_quotes) {
    index = 0; 
    ap_prefs_set_int (w, "current_index", 0);
  }

  /* Choose and output the quote */
  ret = strdup((gchar *) g_list_nth_data (quotes, index));
  free_string_list (quotes);
  return ret;
}

static void quotation_init (struct widget *w)
{
  time_t the_time;
  char *time_string;
  
  time_string = (char *)malloc(1000);
  the_time = time(NULL);
  strftime (time_string, 999, "%Y-%m-%dT%H:%M:%S+00:00", gmtime (&the_time));

  ap_prefs_add_string_list (w, "quotes", NULL);

  ap_prefs_add_int (w, "current_index", 0);
  ap_prefs_add_int (w, "update_rate", 0);
  ap_prefs_add_string (w, "last_update", time_string);

  free (time_string);
}


struct component quotation =
{
  N_("Quotes"),
  N_("Displays a quotation from a provided selection"),
  "Quote",
  &quotation_generate,
  &quotation_init,
  NULL,
  NULL,
  NULL,
  &quotation_menu
};


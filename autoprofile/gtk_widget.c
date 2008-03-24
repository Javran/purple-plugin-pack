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

#include "widget.h"
#include "request.h"
#include "autoprofile.h"

#include "gtkprefs.h"
#include "gtkimhtml.h"

#define AP_RESPONSE_CHOOSE 98125

static GtkWidget *dialog_box = NULL;
static GtkWidget *dialog_box_contents = NULL;
static GtkWidget *dialog_box_preview = NULL;
static struct widget *dialog_box_widget = NULL;
static GStaticMutex preview_mutex = G_STATIC_MUTEX_INIT;

static GtkWidget *component_dialog = NULL;
static GtkWidget *choose_button = NULL;

static GtkWidget *widget_dialog = NULL;
static GtkWidget *delete_button = NULL;
static GtkWidget *rename_button = NULL;
static GtkListStore *tree_list = NULL; 

static GHashTable *pref_names = NULL;

static void component_dialog_show ();

void ap_widget_prefs_updated (struct widget *w) {
  gchar *output;
        
  if (dialog_box_preview == NULL) return;
  if (w != dialog_box_widget) return;

  // TODO: Investigate how laggy this is, possibly add a timeout
  output = w->component->generate (w);
  g_static_mutex_lock (&preview_mutex);
  gtk_imhtml_clear (GTK_IMHTML(dialog_box_preview));
  gtk_imhtml_append_text (GTK_IMHTML(dialog_box_preview), output,
    GTK_IMHTML_NO_SCROLL);
  g_static_mutex_unlock (&preview_mutex);
  free (output);
}

static void update_widget_list (GtkListStore *ls) {
  GtkTreeIter iter;
  GList *widgets, *widgets_start;
  struct widget *w;
  GString *s;

  s = g_string_new ("");

  gtk_list_store_clear (ls);

  widgets_start = widgets = ap_widget_get_widgets ();

  for (widgets = widgets_start; widgets != NULL; widgets = widgets->next) {
    gtk_list_store_append (ls, &iter);
    w = (struct widget *) widgets->data;
    g_string_printf (s, "<b>%s</b>", w->alias);

    gtk_list_store_set (ls, &iter,
      0, s->str,
      1, w,
      -1);
  }
  g_list_free (widgets_start);
  g_string_free (s, TRUE);
}

static void refresh_cb (GtkWidget *widget, gpointer data) {
  struct widget *w;

  w = (struct widget *) data;
  ap_widget_prefs_updated (w);
}

/* Widget configuration menu */
static GtkWidget *get_widget_configuration (struct widget *w) {
  GtkWidget *config, *hbox, *vbox, *sw;
  GtkWidget *label, *button;
  GtkWidget *menu;
  GString *s;
  gchar *output;
  
  config = gtk_vbox_new (FALSE, 0);
 
  /* Title/Description */
  hbox = gtk_hbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER(hbox), 6);
  gtk_box_pack_start (GTK_BOX(config), hbox, FALSE, FALSE, 0);

  s = g_string_new ("");
  g_string_printf (s, "<b>%s:</b> %s", w->component->name, 
    w->component->description);
  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL(label), s->str);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  g_string_free (s, TRUE); 

  /* Separator */
  gtk_box_pack_start (GTK_BOX (config), gtk_hseparator_new (),
    FALSE, FALSE, 0);
 
  /* Preview window */
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER(vbox), 6);
  gtk_box_pack_start (GTK_BOX(config), vbox, FALSE, FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL(label), _("<b>Preview</b>"));
  gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 0);

  button = gtk_button_new_with_label (_("Refresh"));
  gtk_box_pack_end (GTK_BOX(hbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (refresh_cb), w);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(sw),
    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(sw), 
    GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX(vbox), sw, TRUE, TRUE, 0);

  dialog_box_preview = gtk_imhtml_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER(sw), dialog_box_preview);
  pidgin_setup_imhtml (dialog_box_preview);
  output = w->component->generate (w);
  gtk_imhtml_append_text (GTK_IMHTML(dialog_box_preview),
    output, GTK_IMHTML_NO_SCROLL);
  free (output);
  dialog_box_widget = w;

  /* Separator */
  gtk_box_pack_start (GTK_BOX (config), gtk_hseparator_new (),
    FALSE, FALSE, 0);

  /* Configuration stuff */
  vbox = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER(vbox), 6);
  gtk_box_pack_start (GTK_BOX(config), vbox, TRUE, TRUE, 0);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL(label), _("<b>Configuration</b>"));
  gtk_box_pack_start (GTK_BOX(vbox), label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);

  if (w->component->pref_menu == NULL ||
      (menu = (w->component->pref_menu) (w)) == NULL) {
    label = gtk_label_new (_("No options available for this component"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  } else {
    gtk_box_pack_start (GTK_BOX (vbox), menu, TRUE, TRUE, 0);
  }

  return config;
}

/* Info message */
static GtkWidget *get_info_message () {
  GtkWidget *page;
  GtkWidget *aboutwin;
  GtkWidget *text;

  /* Make the box */
  page = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  /* Window with info */
  aboutwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(aboutwin),
    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(aboutwin), 
    GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX(page), aboutwin, TRUE, TRUE, 0);

  text = gtk_imhtml_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER(aboutwin), text);
  pidgin_setup_imhtml (text);

  /* Info text */
  gtk_imhtml_append_text (GTK_IMHTML(text),
    _("<b><u>Basic info</u></b><br>"), GTK_IMHTML_NO_SCROLL);

  gtk_imhtml_append_text (GTK_IMHTML(text),
    _("A <b>widget</b> is a little piece/snippet of automatically "
      "generated text.  There are all sorts of widgets; each type has "
      "different content (i.e. a random quote, text from a blog, the "
      "song currently playing, etc).<br><br>"), GTK_IMHTML_NO_SCROLL);

  gtk_imhtml_append_text (GTK_IMHTML(text),
    _("To use a widget, simply drag it from the list on the left and "
      "drop it into a profile or status message.  <i>It's that easy!</i>"
      "<br><br>"), GTK_IMHTML_NO_SCROLL);

  gtk_imhtml_append_text (GTK_IMHTML(text),
    _("<b>To edit your profile:</b> "
      "Use the \"Info/profile\" tab in this window.<br>"), 
    GTK_IMHTML_NO_SCROLL);

  gtk_imhtml_append_text (GTK_IMHTML(text),
    _("<b>To edit your available/away/status message:</b> "
      "Use the regular Purple interface built into the bottom of the buddy "
      "list.<br><br>"), GTK_IMHTML_NO_SCROLL);

  gtk_imhtml_append_text (GTK_IMHTML(text),
    _("<b><u>Advanced Tips</u></b><br>"), GTK_IMHTML_NO_SCROLL);

  gtk_imhtml_append_text (GTK_IMHTML(text),
    _("You can insert a widget into a profile or status by typing its name.  "
      "To do this, just type \"[widget-name]\" wherever you want to "
      "place a widget (names of widgets are listed on the left).  <br><br>"
      "<b>You type:</b> The song I am playing now is [iTunesInfo].<br>"
      "<b>AutoProfile result:</b> The song I am playing now is "
      "The Beatles - Yellow Submarine.<br><br>"), GTK_IMHTML_NO_SCROLL);

  return page;
}

/* Dialog window actions */
static void widget_popup_rename_cb (
  struct widget *w, const char *new_text) {

  GtkTreeIter iter;
  GValue val;
  struct widget *cur_widget;
  GString *s;

  gtk_tree_model_get_iter_first (GTK_TREE_MODEL(tree_list), &iter);  

  while (TRUE) {
    val.g_type = 0;
    gtk_tree_model_get_value (GTK_TREE_MODEL(tree_list), &iter, 1, &val);
    cur_widget = g_value_get_pointer(&val);

    if (cur_widget == w) break;

    if (!gtk_tree_model_iter_next (GTK_TREE_MODEL(tree_list), &iter)) {
      purple_notify_error (NULL, NULL,
        N_("Unable to change name"), 
        N_("The specified widget no longer exists."));
      return;
    }
  }

  if (ap_widget_rename (w, new_text)) {
    s = g_string_new ("");
    g_string_printf (s, "<b>%s</b>", w->alias);
    // Set value in ls
    gtk_list_store_set (tree_list, &iter,
      0, s->str,
      1, w,
      -1);
    g_string_free (s, TRUE);
  } else {
    purple_notify_error (NULL, NULL,
      N_("Unable to change name"), 
      N_("The widget name you have specified is already in use."));
  }
}

static void delete_cb (GtkWidget *button, GtkTreeSelection *sel)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GValue val;
  struct widget *w;

  gtk_tree_selection_get_selected (sel, &model, &iter);
  val.g_type = 0;
  gtk_tree_model_get_value (model, &iter, 1, &val);
  w = g_value_get_pointer(&val);
  ap_widget_delete (w);
  gtk_list_store_remove (GTK_LIST_STORE(model), &iter);
}

static void rename_cb (GtkWidget *button, GtkTreeSelection *sel)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GValue val;
  struct widget *w;
        
  gtk_tree_selection_get_selected (sel, &model, &iter);
  val.g_type = 0;
  gtk_tree_model_get_value (model, &iter, 1, &val);
  w = g_value_get_pointer(&val);
	
  purple_request_input(NULL, 
    _("Rename Widget"), NULL,
    _("Enter a new name for this widget."), w->alias, 
    FALSE, FALSE, NULL,
    _("Rename"), G_CALLBACK(widget_popup_rename_cb),
    _("Cancel"), NULL, NULL, NULL, NULL, w);
}

static void add_cb (GtkWidget *button, GtkTreeSelection *sel)
{
  component_dialog_show ();
}
        
void ap_widget_gtk_start () {
  pref_names = g_hash_table_new (g_str_hash, g_str_equal);
}

void ap_widget_gtk_finish () {
  done_with_widget_list ();
  g_hash_table_destroy (pref_names);
  pref_names = NULL;
}

static void widget_sel_cb (GtkTreeSelection *sel, GtkTreeModel *model) {
  GtkTreeIter iter;
  struct widget *w;
  GValue val;

  gtk_widget_destroy (dialog_box_contents);
  
  if (!gtk_tree_selection_get_selected (sel, &model, &iter)) {
    gtk_widget_set_sensitive(rename_button, FALSE);
    gtk_widget_set_sensitive(delete_button, FALSE);
    dialog_box_contents = get_info_message ();
    dialog_box_preview = NULL;
    dialog_box_widget = NULL;
  } else {
    gtk_widget_set_sensitive(rename_button, TRUE);
    gtk_widget_set_sensitive(delete_button, TRUE);

    val.g_type = 0;
    gtk_tree_model_get_value (GTK_TREE_MODEL(tree_list), &iter, 1, &val);
    w = g_value_get_pointer(&val);

    dialog_box_contents = get_widget_configuration (w);
  }

  gtk_box_pack_start (GTK_BOX(dialog_box), dialog_box_contents,
    TRUE, TRUE, 0);
  gtk_widget_show_all (dialog_box);
}

GtkWidget *ap_widget_get_config_page ()
{
  GtkTreeSelection *sel;
  GtkWidget *vbox;
  GtkWidget *add_button;
  
  /* Arrange main parts of window */
  dialog_box = gtk_hbox_new (FALSE, 0);
  
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX(dialog_box), vbox, FALSE, FALSE, 0);
  
  get_widget_list (vbox, &sel);
  g_signal_connect (G_OBJECT (sel), "changed", G_CALLBACK (widget_sel_cb), 
    NULL);

  add_button = gtk_button_new_with_label (_("New Widget"));
  g_signal_connect (G_OBJECT(add_button), "clicked", 
                    G_CALLBACK(add_cb), sel);
  gtk_box_pack_start (GTK_BOX(vbox), add_button, FALSE, FALSE, 0);
  
  rename_button = gtk_button_new_with_label (_("Rename"));  
  gtk_widget_set_sensitive(rename_button, FALSE);
  g_signal_connect (G_OBJECT(rename_button), "clicked", 
                    G_CALLBACK(rename_cb), sel);
  gtk_box_pack_start (GTK_BOX(vbox), rename_button, FALSE, FALSE, 0); 

  delete_button = gtk_button_new_with_label (_("Delete"));
  gtk_widget_set_sensitive(delete_button, FALSE); 
  g_signal_connect (G_OBJECT(delete_button), "clicked", 
                    G_CALLBACK(delete_cb), sel);
  gtk_box_pack_start (GTK_BOX(vbox), delete_button, FALSE, FALSE, 0); 

  dialog_box_contents = get_info_message ();
  gtk_box_pack_start (GTK_BOX(dialog_box), dialog_box_contents,
    TRUE, TRUE, 0);

  return dialog_box;
}

/* DND */
static void
drag_data_get_cb(GtkWidget *widget, GdkDragContext *ctx,
  GtkSelectionData *data, guint info, guint time,
  gpointer user_data)
{
  GtkListStore *ls = (GtkListStore *) user_data;
        
  if (ls == NULL) return;
  
  if (data->target == gdk_atom_intern ("STRING", FALSE)) {
    GtkTreeRowReference *ref;
    GtkTreePath *source_row;
    GtkTreeIter iter;
    GString *s;
    struct widget *w;
    GValue val = {0};

    ref = g_object_get_data (G_OBJECT(ctx), "gtk-tree-view-source-row");
    source_row = gtk_tree_row_reference_get_path (ref);

    if (source_row == NULL) return;

    gtk_tree_model_get_iter(GTK_TREE_MODEL(ls), &iter, source_row);
    gtk_tree_model_get_value(GTK_TREE_MODEL(ls), &iter,
      1, &val);

    w = g_value_get_pointer (&val);
    s = g_string_new ("");
    g_string_printf (s, "[%s]", w->alias);
    gtk_selection_data_set (data, gdk_atom_intern ("STRING", FALSE),
      8, s->str, strlen(s->str)+1);

    g_string_free (s, TRUE);
    gtk_tree_path_free (source_row);
  } 
}

void done_with_widget_list () {
  if (tree_list) {
    g_object_unref (tree_list);
    tree_list = NULL;
  }

  widget_dialog = NULL;
  delete_button = NULL;
  dialog_box = NULL;
  dialog_box_contents = NULL;
  dialog_box_preview = NULL;
  dialog_box_widget = NULL;
  if (component_dialog != NULL) {
    gtk_widget_destroy (component_dialog);
    component_dialog = NULL;
    choose_button = NULL;
  }
}

GtkWidget *get_widget_list (GtkWidget *box, GtkTreeSelection **sel) 
{
  GtkWidget *sw;
  GtkWidget *event_view;
  GtkCellRenderer *rend;
  GtkTreeViewColumn *col;
  GtkTargetEntry gte [] = {{"STRING", 0, GTK_IMHTML_DRAG_STRING}};
 
  if (tree_list == NULL) {
    tree_list = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(tree_list),
      0, GTK_SORT_ASCENDING);
    update_widget_list (tree_list);
    g_object_ref (G_OBJECT(tree_list));
  }
  
  /* List of widgets */
  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(sw), 
    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(sw), 
    GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX(box), sw, TRUE, TRUE, 0);

  event_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(tree_list));
  *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (event_view));

  rend = gtk_cell_renderer_text_new ();
  col = gtk_tree_view_column_new_with_attributes (_("Widget"), rend, 
    "markup", 0, NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (event_view), col);
  gtk_tree_view_column_set_sort_column_id (col, 0);
  gtk_container_add (GTK_CONTAINER(sw), event_view);

  /* Drag and Drop */
  gtk_tree_view_enable_model_drag_source(
    GTK_TREE_VIEW(event_view), GDK_BUTTON1_MASK, gte,
    1, GDK_ACTION_COPY);
  g_signal_connect(G_OBJECT(event_view), "drag-data-get",
    G_CALLBACK(drag_data_get_cb), tree_list);

  return event_view;
}

/*********************************************************
  Component selection window
**********************************************************/

static void add_component (struct component *c) {
  struct widget *w;
  GtkTreeIter iter;
  GString *s;

  w = ap_widget_create (c);

  if (w == NULL) return;

  s = g_string_new ("");

  gtk_list_store_append (tree_list, &iter);
  g_string_printf (s, "<b>%s</b>", w->alias);

  gtk_list_store_set (tree_list, &iter,
    0, s->str,
    1, w,
    -1);
  g_string_free (s, TRUE);
}

static void component_row_activate_cb (GtkTreeView *view, GtkTreePath *path,
  GtkTreeViewColumn *column, gpointer null) 
{
  GtkTreeSelection *sel;
  GtkTreeIter iter;
  struct component *c;
  GtkTreeModel *model;

  sel = gtk_tree_view_get_selection (view);

  if (gtk_tree_selection_get_selected (sel, &model, &iter)) {
    gtk_tree_model_get (model, &iter, 1, &c, -1);
    add_component (c);
  }

  gtk_widget_destroy (component_dialog);
  component_dialog = NULL;
  choose_button = NULL; 
}

static void component_response_cb(GtkWidget *d, int response, 
  GtkTreeSelection *sel)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GValue val;
  struct component *c;

  switch (response) {
    case AP_RESPONSE_CHOOSE:
      gtk_tree_selection_get_selected (sel, &model, &iter);
      val.g_type = 0;
      gtk_tree_model_get_value (model, &iter, 1, &val);
      c = g_value_get_pointer(&val);
      add_component (c);
    case GTK_RESPONSE_CLOSE:
    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_DELETE_EVENT:
      gtk_widget_destroy(d);
      component_dialog = NULL;
      choose_button = NULL;
      break;
  }
}

static void component_sel_cb (GtkTreeSelection *sel, GtkTreeModel *model) {
  GtkTreeIter iter;

  if (!gtk_tree_selection_get_selected (sel, &model, &iter)) {
    gtk_widget_set_sensitive(choose_button, FALSE);
  } else {
    gtk_widget_set_sensitive(choose_button, TRUE);
  }
}

static void update_component_list (GtkListStore *ls) {
  GtkTreeIter iter;
  GList *components;
  struct component *c;
  GString *s;
  gchar *name, *description;

  gtk_list_store_clear (ls);

  s = g_string_new ("");

  for (components = ap_component_get_components ();
       components != NULL;
       components = components->next) {
    gtk_list_store_append (ls, &iter);
    c = (struct component *) components->data;

    name = g_markup_escape_text (c->name, -1);
    description = g_markup_escape_text (c->description, -1);

    g_string_printf (s, "<b>%s</b>\n%s", name, description);

    gtk_list_store_set (ls, &iter,
      0, s->str,
      1, c,
      -1);
    free (name);
    free (description);
  }

  g_string_free (s, TRUE);
}

static void component_dialog_show ()
{
  GtkWidget *sw;
  GtkWidget *event_view;
  GtkListStore *ls;
  GtkCellRenderer *rendt;
  GtkTreeViewColumn *col;
  GtkTreeSelection *sel;

  if (component_dialog != NULL) {
    gtk_window_present(GTK_WINDOW(component_dialog));
    return;
  }

  component_dialog = gtk_dialog_new_with_buttons (_("Select a widget type"),
    NULL,
    GTK_DIALOG_NO_SEPARATOR,
    NULL);

  choose_button = gtk_dialog_add_button (GTK_DIALOG(component_dialog),
    _("Create widget"), AP_RESPONSE_CHOOSE);
  gtk_dialog_add_button (GTK_DIALOG(component_dialog),
    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
  gtk_widget_set_sensitive (choose_button, FALSE);

  sw = gtk_scrolled_window_new (NULL,NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(sw), 
    GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(sw), 
    GTK_SHADOW_IN);

  gtk_box_pack_start (GTK_BOX(GTK_DIALOG(component_dialog)->vbox), sw, 
    TRUE, TRUE, 0);

  ls = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(ls),
    0, GTK_SORT_ASCENDING);

  update_component_list (ls);

  event_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL(ls));

  g_signal_connect(G_OBJECT(event_view), "row-activated",
    G_CALLBACK(component_row_activate_cb), event_view);

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (event_view));

  rendt = gtk_cell_renderer_text_new ();
  col = gtk_tree_view_column_new_with_attributes (_("Widget type"),
    rendt,
    "markup", 0,
    NULL);

#if GTK_CHECK_VERSION(2,6,0)
  gtk_tree_view_column_set_expand (col, TRUE);
  g_object_set(rendt, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
#endif
  gtk_tree_view_append_column (GTK_TREE_VIEW(event_view), col);
  gtk_tree_view_column_set_sort_column_id (col, 0);
  g_object_unref (G_OBJECT(ls));
  gtk_container_add (GTK_CONTAINER(sw), event_view);

  g_signal_connect (G_OBJECT (sel), "changed", 
    G_CALLBACK (component_sel_cb), NULL);
  g_signal_connect(G_OBJECT(component_dialog), "response", 
    G_CALLBACK(component_response_cb), sel);
  gtk_window_set_default_size(GTK_WINDOW(component_dialog), 550, 430);
  gtk_widget_show_all(component_dialog);
}

/* Preferences stuff */
static void pref_callback (const char *name, PurplePrefType type,
  gconstpointer val, gpointer data) {
  struct widget *w = (struct widget *) data;
  ap_widget_prefs_updated (w);
}

static const gchar *get_const_pref (struct widget *w, const char *key) {
  gchar *pref, *result;
  // This is here to prevent memory leaks

  pref = ap_prefs_get_pref_name (w, key);
  if (pref_names == NULL) {
    pref_names = g_hash_table_new (g_str_hash, g_str_equal);
  }

  result = g_hash_table_lookup (pref_names, pref);

  if (!result) {
    g_hash_table_insert (pref_names, pref, pref);
    return pref;
  } else {
    free (pref);
    return result;
  }
}

GtkWidget *ap_prefs_checkbox (struct widget *w, const char *title, 
  const char *key, GtkWidget *page) 
{
  GtkWidget *result;
  const gchar *pref;

  pref = get_const_pref (w, key);
  result = pidgin_prefs_checkbox (title, pref, page);
  purple_prefs_connect_callback (ap_get_plugin_handle (), pref,
    pref_callback, w);

  return result;
}

GtkWidget *ap_prefs_dropdown_from_list (struct widget *w, GtkWidget *page,
  const gchar *title, PurplePrefType type, const char *key, GList *menuitems) 
{
  GtkWidget *result;
  const gchar *pref;

  pref = get_const_pref (w, key);
  result = pidgin_prefs_dropdown_from_list (
    page, title, type, pref, menuitems);
  purple_prefs_connect_callback (ap_get_plugin_handle (), pref,
    pref_callback, w);

  return result;
}

GtkWidget *ap_prefs_labeled_entry (struct widget *w, GtkWidget *page, 
  const gchar *title, const char *key, GtkSizeGroup *sg) {
  GtkWidget *result; 
  const gchar *pref;

  pref = get_const_pref (w, key);
  result = pidgin_prefs_labeled_entry (page, title, pref, sg);
  purple_prefs_connect_callback (ap_get_plugin_handle (), pref,
    pref_callback, w);

  return result;
}

GtkWidget *ap_prefs_labeled_spin_button (struct widget *w, 
  GtkWidget *page, const gchar *title, const char *key, int min, 
  int max, GtkSizeGroup *sg) 
{
  GtkWidget *result; 
  const gchar *pref;

  pref = get_const_pref (w, key);
  result = pidgin_prefs_labeled_spin_button (page, title, pref,
    min, max, sg);
  purple_prefs_connect_callback (ap_get_plugin_handle (), pref,
    pref_callback, w);

  return result;
}


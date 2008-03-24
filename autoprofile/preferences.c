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

#include "gtkimhtml.h"
#include "gtksavedstatuses.h"
#include "gtkprefs.h"

/*--------------------------------------------------------------------------*
 * Info Tab                                                                 *
 *--------------------------------------------------------------------------*/
static GtkWidget *get_info_page () {
  GtkWidget *page;
  GtkWidget *label;
  GtkWidget *aboutwin;
  GtkWidget *text;

  gchar *labeltext, *str;

  /* Make the box */
  page = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (page), 5);

  /* AutoProfile title */
  labeltext = g_strdup_printf (
    _("<span weight=\"bold\" size=\"larger\">AutoProfile %s</span>"),
    PP_VERSION);
  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL(label), labeltext);
  gtk_label_set_line_wrap (GTK_LABEL(label), TRUE);
  gtk_misc_set_alignment (GTK_MISC(label), 0.5, 0);
  gtk_box_pack_start (GTK_BOX(page), label, FALSE, FALSE, 0);
  g_free(labeltext);

  /* Window with info */
  aboutwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(aboutwin),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(aboutwin), 
    GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX(page), aboutwin, TRUE, TRUE, 0);

  text = gtk_imhtml_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER(aboutwin), text);
  pidgin_setup_imhtml (text);

  /* Info text */
  gtk_imhtml_append_text (GTK_IMHTML(text),
    _("Use the <b>Autoprofile</b> portion of the <b>Tools</b> "
      "menu in the <b>"
      "buddy list</b> to configure the actual content that will go in your "
      "status messages and profiles and set options.<br><br>"),
    GTK_IMHTML_NO_SCROLL);

  gtk_imhtml_append_text (GTK_IMHTML(text),
    _("<u>DOCUMENTATION / HELP</u><br>"), GTK_IMHTML_NO_SCROLL);
  gtk_imhtml_append_text (GTK_IMHTML(text),
    _("Complete documentation can be found at:<br> <a href="
    "\"http://hkn.eecs.berkeley.edu/~casey/autoprofile/documentation.php\">"
    "hkn.eecs.berkeley.edu/~casey/autoprofile/documentation.php</a><br>"),
    GTK_IMHTML_NO_SCROLL);

  gtk_imhtml_append_text (GTK_IMHTML(text),
    _("<br><u>ABOUT</u><br>"), GTK_IMHTML_NO_SCROLL);

  str = g_strconcat(
    "<font size=\"3\"><b>", _("Developers"), ":</b></font><br>"
    "  Casey Ho (Lead Developer)<br>"
    "  Mitchell Harwell<br>", NULL);
  gtk_imhtml_append_text(GTK_IMHTML(text), str, GTK_IMHTML_NO_SCROLL);
  g_free(str);

  str = g_strconcat(
    "<font size=\"3\"><b>", _("Contributors/Patchers"), ":</b></font><br>"
    "  Onime Clement<br>"
    "  Michael Milligan<br>"
    "  Mark Painter<br>", NULL);
  gtk_imhtml_append_text(GTK_IMHTML(text), str, GTK_IMHTML_NO_SCROLL);
  g_free(str);

  str = g_strconcat(
    "<font size=\"3\"><b>", _("Website"), ":</b></font><br>"
    "  <a href=\"http://autoprofile.sourceforge.net\">"
    "autoprofile.sourceforge.net<br>", NULL);
  gtk_imhtml_append_text(GTK_IMHTML(text), str, GTK_IMHTML_NO_SCROLL);
  g_free(str);

  return page;
}

/*----------------------------------------------------------------------------
 * Accounts Tab
 *--------------------------------------------------------------------------*/
/* PRIMARILY RIPPED FROM GAIM GTKACCOUNT.C */
enum
{
  COLUMN_ICON,
  COLUMN_SCREENNAME,
  COLUMN_ENABLED,
  COLUMN_PROTOCOL,
  COLUMN_DATA,
  COLUMN_PULSE_DATA,
  NUM_COLUMNS
};

typedef struct
{
  PurpleAccount *account;
  char *username;
  char *alias;
} PidginAccountAddUserData;

typedef struct
{
  GtkWidget *treeview;

  GtkListStore *model;
  GtkTreeIter drag_iter;

  GtkTreeViewColumn *screenname_col;
} AccountsWindow;

static void add_account_to_liststore(PurpleAccount *, gpointer);
static void set_account(GtkListStore *, GtkTreeIter *, PurpleAccount *);

static gboolean is_profile_settable (PurpleAccount *a) {
  const gchar *id = purple_account_get_protocol_id (a);
  if (!strcmp (id, "prpl-yahoo") ||
      !strcmp (id, "prpl-msn") ||
      !strcmp (id, "prpl-jabber")) {
    return FALSE;
  }

  return TRUE;
}

static void
drag_data_get_cb(GtkWidget *widget, GdkDragContext *ctx,
  GtkSelectionData *data, guint info, guint time,
  AccountsWindow *dialog)
{
  if (data->target == gdk_atom_intern("PURPLE_ACCOUNT", FALSE)) {
    GtkTreeRowReference *ref;
    GtkTreePath *source_row;
    GtkTreeIter iter;
    PurpleAccount *account = NULL;
    GValue val = {0};

    ref = g_object_get_data(G_OBJECT(ctx), "gtk-tree-view-source-row");
    source_row = gtk_tree_row_reference_get_path(ref);

    if (source_row == NULL) return; 

    gtk_tree_model_get_iter(GTK_TREE_MODEL(dialog->model), &iter, source_row);
    gtk_tree_model_get_value(GTK_TREE_MODEL(dialog->model), &iter,
      COLUMN_DATA, &val);

    dialog->drag_iter = iter;

    account = g_value_get_pointer(&val);

    gtk_selection_data_set(data, gdk_atom_intern("PURPLE_ACCOUNT", FALSE),
      8, (void *)&account, sizeof(account));

    gtk_tree_path_free(source_row);
  }
}

static void
move_account_after(GtkListStore *store, GtkTreeIter *iter,
  GtkTreeIter *position)
{
  GtkTreeIter new_iter;
  PurpleAccount *account;

  gtk_tree_model_get(GTK_TREE_MODEL(store), iter, COLUMN_DATA, &account, -1);
  gtk_list_store_insert_after(store, &new_iter, position);

  set_account(store, &new_iter, account);

  gtk_list_store_remove(store, iter);
}

static void
move_account_before(GtkListStore *store, GtkTreeIter *iter,
  GtkTreeIter *position)
{
  GtkTreeIter new_iter;
  PurpleAccount *account;

  gtk_tree_model_get(GTK_TREE_MODEL(store), iter, COLUMN_DATA, &account, -1);
  gtk_list_store_insert_before(store, &new_iter, position);

  set_account(store, &new_iter, account);

  gtk_list_store_remove(store, iter);
}

static void
drag_data_received_cb(GtkWidget *widget, GdkDragContext *ctx,
  guint x, guint y, GtkSelectionData *sd,
  guint info, guint t, AccountsWindow *dialog)
{
  if (sd->target == gdk_atom_intern("PURPLE_ACCOUNT", FALSE) && sd->data) {
    gint dest_index;
    PurpleAccount *a = NULL;
    GtkTreePath *path = NULL;
    GtkTreeViewDropPosition position;

    memcpy(&a, sd->data, sizeof(a));

    if (gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget), x, y,
      &path, &position)) {

      GtkTreeIter iter;
      PurpleAccount *account;
      GValue val = {0};

      gtk_tree_model_get_iter(GTK_TREE_MODEL(dialog->model), &iter, path);
      gtk_tree_model_get_value(GTK_TREE_MODEL(dialog->model), &iter,
        COLUMN_DATA, &val);

      account = g_value_get_pointer(&val);

      switch (position) {
        case GTK_TREE_VIEW_DROP_AFTER:
        case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
          move_account_after(dialog->model, &dialog->drag_iter, &iter);
          dest_index = g_list_index(purple_accounts_get_all(), account) + 1;
          break;

        case GTK_TREE_VIEW_DROP_BEFORE:
        case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
          dest_index = g_list_index(purple_accounts_get_all(), account);
          move_account_before(dialog->model, &dialog->drag_iter, &iter);
          break;

        default:
        return;
      }

      purple_accounts_reorder(a, dest_index);
    }
  }
}

static void
enabled_cb(GtkCellRendererToggle *renderer, gchar *path_str, gpointer data)
{
  AccountsWindow *dialog = (AccountsWindow *)data;
  PurpleAccount *account;
  GtkTreeModel *model = GTK_TREE_MODEL(dialog->model);
  GtkTreeIter iter;
  gboolean enabled;

  gtk_tree_model_get_iter_from_string(model, &iter, path_str);
  gtk_tree_model_get(model, &iter,
    COLUMN_DATA, &account,
    COLUMN_ENABLED, &enabled,
    -1);

  /* Change profile settings */
  ap_account_enable_profile (account, !enabled);
  set_account (dialog->model, &iter, account);
}

static void
add_columns(GtkWidget *treeview, AccountsWindow *dialog)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* Screen Name column */
  column = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(column, _("Screen Name"));
  gtk_tree_view_insert_column(GTK_TREE_VIEW(treeview), column, -1);
  gtk_tree_view_column_set_resizable(column, TRUE);

  /* Icon */
  renderer = gtk_cell_renderer_pixbuf_new();
  gtk_tree_view_column_pack_start(column, renderer, FALSE);
  gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", COLUMN_ICON);

  /* Screen Name */
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(column, renderer, TRUE);
  gtk_tree_view_column_add_attribute(column, renderer,
    "text", COLUMN_SCREENNAME);
  dialog->screenname_col = column;

  /* Enabled */
  renderer = gtk_cell_renderer_toggle_new();

  g_signal_connect(G_OBJECT(renderer), "toggled",
    G_CALLBACK(enabled_cb), dialog);

  column = 
    gtk_tree_view_column_new_with_attributes(_("AutoProfile sets user info"),
    renderer, "active", COLUMN_ENABLED, NULL);

  gtk_tree_view_insert_column(GTK_TREE_VIEW(treeview), column, -1);
  gtk_tree_view_column_set_resizable(column, TRUE);

  /* Protocol name */
  column = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(column, _("Protocol"));
  gtk_tree_view_insert_column(GTK_TREE_VIEW(treeview), column, -1);
  gtk_tree_view_column_set_resizable(column, TRUE);

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(column, renderer, TRUE);
  gtk_tree_view_column_add_attribute(column, renderer,
    "text", COLUMN_PROTOCOL);
}

static void
set_account(GtkListStore *store, GtkTreeIter *iter, PurpleAccount *account)
{
  GdkPixbuf *pixbuf;
  GdkPixbuf *scale;

  scale = NULL;

  pixbuf = pidgin_create_prpl_icon(account, 0.5);

  if (pixbuf != NULL)
  {
    scale = gdk_pixbuf_scale_simple(pixbuf, 16, 16, GDK_INTERP_BILINEAR);

    if (purple_account_is_disconnected(account))
      gdk_pixbuf_saturate_and_pixelate(scale, scale, 0.0, FALSE);
  }

  gtk_list_store_set(store, iter,
    COLUMN_ICON, scale,
    COLUMN_SCREENNAME, purple_account_get_username(account),
    COLUMN_ENABLED, ap_account_has_profile_enabled(account),
    COLUMN_PROTOCOL, purple_account_get_protocol_name(account),
    COLUMN_DATA, account,
    -1);

  if (pixbuf != NULL) g_object_unref(G_OBJECT(pixbuf));
  if (scale  != NULL) g_object_unref(G_OBJECT(scale));
}

static void
add_account_to_liststore(PurpleAccount *account, gpointer user_data)
{
  GtkTreeIter iter;
  AccountsWindow *dialog = (AccountsWindow *) user_data;

  if (dialog == NULL) return;

  if (!is_profile_settable (account)) return;

  gtk_list_store_append(dialog->model, &iter);
  set_account(dialog->model, &iter, account);
}

static void
populate_accounts_list(AccountsWindow *dialog)
{
  GList *l;

  gtk_list_store_clear(dialog->model);

  for (l = purple_accounts_get_all(); l != NULL; l = l->next)
    add_account_to_liststore((PurpleAccount *)l->data, dialog);
  }

#if !GTK_CHECK_VERSION(2,2,0)
static void
get_selected_helper(GtkTreeModel *model, GtkTreePath *path,
  GtkTreeIter *iter, gpointer user_data)
{
  *((gboolean *)user_data) = TRUE;
}
#endif

static void
account_selected_cb(GtkTreeSelection *sel, AccountsWindow *dialog)
{
  gboolean selected = FALSE;

#if GTK_CHECK_VERSION(2,2,0)
  selected = (gtk_tree_selection_count_selected_rows(sel) > 0);
#else
  gtk_tree_selection_selected_foreach(sel, get_selected_helper, &selected);
#endif
}

static GtkWidget *
create_accounts_list(AccountsWindow *dialog)
{
  GtkWidget *sw;
  GtkWidget *treeview;
  GtkTreeSelection *sel;
  GtkTargetEntry gte[] = {{"PURPLE_ACCOUNT", GTK_TARGET_SAME_APP, 0}};

  /* Create the scrolled window. */
  sw = gtk_scrolled_window_new(0, 0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
    GTK_POLICY_AUTOMATIC,
    GTK_POLICY_ALWAYS);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_IN);
  gtk_widget_show(sw);

  /* Create the list model. */
  dialog->model = gtk_list_store_new(NUM_COLUMNS,
    GDK_TYPE_PIXBUF, G_TYPE_STRING,
    G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_POINTER,
    G_TYPE_POINTER);

  /* And now the actual treeview */
  treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(dialog->model));
  dialog->treeview = treeview;
  gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), TRUE);

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
  gtk_tree_selection_set_mode(sel, GTK_SELECTION_MULTIPLE);
  g_signal_connect(G_OBJECT(sel), "changed",
    G_CALLBACK(account_selected_cb), dialog);

  gtk_container_add(GTK_CONTAINER(sw), treeview);
  gtk_widget_show(treeview);

  add_columns(treeview, dialog);

  populate_accounts_list(dialog);

  /* Setup DND. I wanna be an orc! */
  gtk_tree_view_enable_model_drag_source(
    GTK_TREE_VIEW(treeview), GDK_BUTTON1_MASK, gte,
    1, GDK_ACTION_COPY);
  gtk_tree_view_enable_model_drag_dest(
    GTK_TREE_VIEW(treeview), gte, 1,
    GDK_ACTION_COPY | GDK_ACTION_MOVE);

  g_signal_connect(G_OBJECT(treeview), "drag-data-received",
    G_CALLBACK(drag_data_received_cb), dialog);
  g_signal_connect(G_OBJECT(treeview), "drag-data-get",
    G_CALLBACK(drag_data_get_cb), dialog);

  return sw;
}

static void account_page_delete_cb (GtkObject *object, gpointer data)
{
  g_free (data);
}

GtkWidget *get_account_page () {
  GtkWidget *page;
  GtkWidget *sw;
  GtkWidget *label;
  AccountsWindow *accounts_window;

  /* Make the box */
  page = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  accounts_window = g_new0(AccountsWindow, 1);

  /* Setup the scrolled window that will contain the list of accounts. */
  sw = create_accounts_list(accounts_window);
  gtk_box_pack_start(GTK_BOX(page), sw, TRUE, TRUE, 0);

  label = gtk_label_new (
    _("Accounts that do not support user-specified profiles are not shown"));
  gtk_box_pack_start(GTK_BOX(page), label, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (page), "destroy",
    G_CALLBACK (account_page_delete_cb), accounts_window);

  return page;
}

/*----------------------------------------------------------------------------
 * Behavior Tab
 *--------------------------------------------------------------------------*/
void ap_gtk_prefs_add_summary_option (GtkWidget *widget) {
  pidgin_prefs_dropdown (widget,
    "Show AutoProfile summary window",
    PURPLE_PREF_STRING,
    "/plugins/gtk/autoprofile/show_summary",
    "Always", "always", "When away", "away", "Never", "never", NULL);
}

static void
set_idle_away(PurpleSavedStatus *status)
{
  purple_prefs_set_int("/core/savedstatus/idleaway", 
    purple_savedstatus_get_creation_time(status));
}

static GtkWidget *get_behavior_page () {
  GtkWidget *page;
  GtkWidget *label;
  GtkWidget *frame, *vbox, *hbox;
  GtkWidget *button, *select, *menu;
  GtkSizeGroup *sg;
  gchar *markup;

  /* Make the box */
  page = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  /*---------- Update frequency ----------*/
  frame = pidgin_make_frame (page, _("Update frequency"));
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  pidgin_prefs_labeled_spin_button (vbox,
    _("Minimum number of seconds between updates"),
    "/plugins/gtk/autoprofile/delay_update",
    15, 1000, NULL);

  label = gtk_label_new ("");
  markup = g_markup_printf_escaped ("<span style=\"italic\">%s</span>", 
    _("WARNING: Using values below 60 seconds may increase the frequency\n"
      "of rate limiting errors"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  /*----------- Auto-away stuff ------------*/
  frame = pidgin_make_frame(page, _("Auto-away"));

  button = pidgin_prefs_checkbox(_("Change status when idle"),
    "/plugins/gtk/autoprofile/away_when_idle", frame);

  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  select = pidgin_prefs_labeled_spin_button(frame,
    _("Minutes before changing status:"), "/core/away/mins_before_away",
    1, 24 * 60, sg);
  g_signal_connect(G_OBJECT(button), "clicked",
    G_CALLBACK(pidgin_toggle_sensitive), select);

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), hbox);

  label = gtk_label_new_with_mnemonic(_("Change status to:"));
  gtk_size_group_add_widget(sg, label);
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  g_signal_connect(G_OBJECT(button), "clicked",
    G_CALLBACK(pidgin_toggle_sensitive), label);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

  /* TODO: Show something useful if we don't have any saved statuses. */
  menu = pidgin_status_menu(purple_savedstatus_get_idleaway(), 
    G_CALLBACK(set_idle_away));
  gtk_box_pack_start(GTK_BOX(frame), menu, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(button), "clicked",
    G_CALLBACK(pidgin_toggle_sensitive), menu);
  gtk_label_set_mnemonic_widget(GTK_LABEL(label), menu);

  if (!purple_prefs_get_bool("/plugins/gtk/autoprofile/away_when_idle")) {
    gtk_widget_set_sensitive(GTK_WIDGET(menu), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(select), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(label), FALSE);
  }

  return page;
}

/*----------------------------------------------------------------------------
 * Auto-reply Tab
 *--------------------------------------------------------------------------*/
/* Update string arguments */
static gboolean update_behavior_string (GtkWidget *widget, GdkEventFocus *evt,
  gpointer data)
{
  ap_debug ("preferences", "behavior string preference modified");

  if (!strcmp (data, "text_trigger")) {
    purple_prefs_set_string ("/plugins/gtk/autoprofile/autorespond/trigger",
      gtk_entry_get_text (GTK_ENTRY (widget)));
  } else if (!strcmp (data, "text_respond")) {
    purple_prefs_set_string ("/plugins/gtk/autoprofile/autorespond/text",
      gtk_entry_get_text (GTK_ENTRY (widget)));
  } else {
    ap_debug_error ("preferences", "invalid data argument to string update");
  }

  return FALSE;
}

/* Update value returned from spinner for auto-respond delay */
static gboolean update_delay_respond (GtkWidget *widget, GdkEventFocus *evt, 
                                      gpointer data)
{
  purple_prefs_set_int ("/plugins/gtk/autoprofile/delay_respond",
    gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget)));
  return FALSE;
}

static GtkWidget *get_autoreply_page () {
  GtkWidget *page;
  GtkWidget *label, *checkbox, *spinner, *entry;
  GtkWidget *frame, *vbox, *large_vbox, *hbox;
  GtkWidget *dd;
  GtkSizeGroup *sg;

  /* Make the box */
  page = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

	frame = pidgin_make_frame(page, _("General"));

	dd = pidgin_prefs_dropdown(frame, _("Auto-reply:"),
		PURPLE_PREF_STRING, "/plugins/gtk/autoprofile/autorespond/auto_reply",
		_("Never"), "never",
		_("When away"), "away",
		_("When both away and idle"), "awayidle",
		NULL);
  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget(sg, dd);
	gtk_misc_set_alignment(GTK_MISC(dd), 0, 0.5);
 
  /*---------- Auto-responses ----------*/
  frame = pidgin_make_frame (page, _("Dynamic auto-responses"));
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /* Auto-response activated */
  checkbox = pidgin_prefs_checkbox (
    _("Allow users to request more auto-responses"),
    "/plugins/gtk/autoprofile/autorespond/enable", vbox);
  large_vbox = gtk_vbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), large_vbox, FALSE, FALSE, 0);
 
  /* Auto-response delay */
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (large_vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Delay"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  spinner = gtk_spin_button_new_with_range (1, G_MAXINT, 1);
  gtk_box_pack_start (GTK_BOX (hbox), spinner, TRUE, TRUE, 0); 
  label = gtk_label_new (_("seconds between auto-responses"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinner), purple_prefs_get_int (
    "/plugins/gtk/autoprofile/autorespond/delay"));
  g_signal_connect (G_OBJECT (spinner), "value-changed",
                    G_CALLBACK (update_delay_respond), NULL);

  /* Auto-response message string */
  label = gtk_label_new (_("Message sent with first autoresponse:"));
  gtk_box_pack_start (GTK_BOX (large_vbox), label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (large_vbox), entry, FALSE, FALSE, 0);
  gtk_entry_set_max_length (GTK_ENTRY (entry), 100);
  gtk_entry_set_text (GTK_ENTRY (entry), purple_prefs_get_string (
    "/plugins/gtk/autoprofile/autorespond/text"));
  g_signal_connect (G_OBJECT (entry), "focus-out-event",
                    G_CALLBACK (update_behavior_string), "text_respond");
  
  label = gtk_label_new (_("Request trigger message:"));
  gtk_box_pack_start (GTK_BOX (large_vbox), label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (large_vbox), entry, FALSE, FALSE, 0);
  gtk_entry_set_max_length (GTK_ENTRY (entry), 50);
  gtk_entry_set_text (GTK_ENTRY (entry), purple_prefs_get_string (
    "/plugins/gtk/autoprofile/autorespond/trigger"));
  g_signal_connect (G_OBJECT (entry), "focus-out-event",
                    G_CALLBACK (update_behavior_string), "text_trigger");

  /* Sensitivity signals */
  g_signal_connect(G_OBJECT(checkbox), "clicked",
    G_CALLBACK(pidgin_toggle_sensitive), large_vbox);
  if (!purple_prefs_get_bool ("/plugins/gtk/autoprofile/autorespond/enable")) {
    gtk_widget_set_sensitive (large_vbox, FALSE);
  } else {
    gtk_widget_set_sensitive (large_vbox, TRUE);
  }

  return page;
}

/*----------------------------------------------------------------------------
 * Menu as a whole
 *--------------------------------------------------------------------------*/
static GtkWidget *get_config_frame (PurplePlugin *plugin)
{
  GtkWidget *info = get_info_page ();
  gtk_widget_set_size_request (info, 350, 400);
  return info;
}

static void dialog_cb (GtkDialog *dialog, gint arg1, gpointer user_data)
{
  gtk_widget_destroy ((GtkWidget *)dialog);
}

void ap_preferences_display ()
{
  GtkWidget *dialog, *notebook;

  notebook = gtk_notebook_new ();

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
    get_behavior_page (),   gtk_label_new (_("General")));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
    get_account_page (),    gtk_label_new (_("User info/profiles")));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
    get_autoreply_page (),  gtk_label_new (_("Auto-reply")));

  g_object_set (notebook, "homogeneous", TRUE, NULL); 

  dialog = gtk_dialog_new_with_buttons(PIDGIN_ALERT_TITLE, NULL,
    GTK_DIALOG_NO_SEPARATOR,
    GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
    NULL);

  gtk_container_add (GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), notebook);
  gtk_window_set_default_size (GTK_WINDOW(dialog), 400, 400);
  gtk_widget_show_all (dialog);

  g_signal_connect (G_OBJECT(dialog), "response",
    G_CALLBACK(dialog_cb), dialog);
}

/*--------------- Generate the preference widget once ----------------*/
PidginPluginUiInfo ui_info = 
{ 
  get_config_frame
};


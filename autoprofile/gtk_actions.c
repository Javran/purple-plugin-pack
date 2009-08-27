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
#include "gtkimhtmltoolbar.h"

static GtkWidget *current_profile = NULL;
static GtkWidget *accounts_dialog = NULL;
static GtkWidget *content_win = NULL;

/*--------------------------------------------------------------------------* 
 * Accounts edit popup window                                               *
 *--------------------------------------------------------------------------*/
static void accounts_response_cb (GtkWidget *d, int response, gpointer data)
{
  gtk_widget_destroy (accounts_dialog);
  accounts_dialog = NULL;
}

static void display_accounts_dialog () {
  GtkWidget *label;
 
  if (accounts_dialog != NULL) {
    gtk_window_present (GTK_WINDOW (accounts_dialog));
    return;
  }

  accounts_dialog = gtk_dialog_new_with_buttons (_("Edit Profile Accounts"),
    NULL, GTK_DIALOG_NO_SEPARATOR, NULL, NULL);
  gtk_dialog_set_has_separator (GTK_DIALOG(accounts_dialog), TRUE);

  gtk_dialog_add_button (GTK_DIALOG(accounts_dialog), GTK_STOCK_OK, 
    GTK_RESPONSE_OK);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), 
    _("<b>No accounts currently enabled:</b> You have not yet specified\n "
      "what accounts AutoProfile should set the profile for.  Until you\n "
      "check one of the boxes below, AutoProfile will effectively do\n "
      "nothing."));
  gtk_box_pack_start (GTK_BOX(GTK_DIALOG(accounts_dialog)->vbox), label,
    FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(GTK_DIALOG(accounts_dialog)->vbox),
    get_account_page (), TRUE, TRUE, 0);
  
  g_signal_connect (G_OBJECT(accounts_dialog), "response",
    G_CALLBACK(accounts_response_cb), NULL);
  gtk_window_set_default_size (GTK_WINDOW(accounts_dialog), 400, 450);
  gtk_widget_show_all (accounts_dialog);
}

/*--------------------------------------------------------------------------* 
 * Profile edit window                                                      *
 *--------------------------------------------------------------------------*/
/* Callbacks for refreshing profile preview window */
static void refresh_preview (GtkWidget *preview) {
  // TODO: See if a delay timeout is necessary here

  gchar *output, *input;

  if (preview == NULL || current_profile == NULL) return;
 
  gtk_imhtml_clear (GTK_IMHTML(preview));
  input = gtk_imhtml_get_markup ((GtkIMHtml *) current_profile);
  output = ap_generate (input, AP_SIZE_PROFILE_MAX);
  gtk_imhtml_append_text (GTK_IMHTML(preview), output,
    GTK_IMHTML_NO_SCROLL);
  free (input);
  free (output);
}

static void refresh_cb (GtkWidget *widget, gpointer data) 
{
  refresh_preview ((GtkWidget *) data);  
}

static void event_cb (GtkWidget *widget, gpointer data)
{
  refresh_preview ((GtkWidget *) data);
}

static void formatting_toggle_cb (GtkIMHtml *imhtml, 
                GtkIMHtmlButtons buttons, gpointer data)
{
  refresh_preview ((GtkWidget *) data);
}

static void formatting_clear_cb (GtkIMHtml *imhtml, gpointer data)
{
  refresh_preview ((GtkWidget *) data);
}

static void revert_cb (GtkWidget *button, GtkWidget *imhtml)
{
  gtk_imhtml_clear (GTK_IMHTML(imhtml));
  gtk_imhtml_append_text_with_images (GTK_IMHTML(imhtml),
                  purple_prefs_get_string ("/plugins/gtk/autoprofile/profile"),
                  0, NULL);
}

static void save_cb (GtkWidget *button, GtkWidget *imhtml)
{
  gchar *new_text;

  if (imhtml == NULL) return;  

  new_text = gtk_imhtml_get_markup ((GtkIMHtml *) imhtml);
  purple_prefs_set_string ("/plugins/gtk/autoprofile/profile", new_text);
  free (new_text);
    
  if (NULL == purple_prefs_get_string_list (  
        "/plugins/gtk/autoprofile/profile_accounts")) {
    // If no accounts set, ask for one!
    display_accounts_dialog ();
  }
}

/* Profile edit window */
static GtkWidget *get_profile_page ()
{
  GtkTreeSelection *sel;

  GtkWidget *ret;
  GtkWidget *hbox, *vbox, *dialog_box, *preview, *edit_window;
  GtkWidget *label, *sw, *toolbar, *bbox;
  GtkWidget *refresh_button, *revert_button, *save_button;
  GtkTextBuffer *text_buffer;
 
  ret = gtk_vbox_new (FALSE, 6);

  /* Preview window */
  dialog_box = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER(dialog_box), 6);
  gtk_box_pack_start (GTK_BOX(ret), dialog_box, TRUE, TRUE, 0);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX(dialog_box), hbox, FALSE, FALSE, 0);
 
  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL(label), _("<b>Preview</b>"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 0);

  refresh_button = gtk_button_new_with_label (_("Refresh"));
  gtk_box_pack_end (GTK_BOX(hbox), refresh_button, FALSE, FALSE, 0);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(sw),
    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(sw), 
    GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX(dialog_box), sw, TRUE, TRUE, 0);

  preview = gtk_imhtml_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER(sw), preview);
  pidgin_setup_imhtml (preview);
  gtk_imhtml_append_text (GTK_IMHTML(preview),
    purple_prefs_get_string ("/plugins/gtk/autoprofile/profile"),
    GTK_IMHTML_NO_SCROLL);

  /* Separator */
  gtk_box_pack_start (GTK_BOX(ret), gtk_hseparator_new (), FALSE, FALSE, 0); 

  /* Edit window */
  dialog_box = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER(dialog_box), 6);
  gtk_box_pack_start (GTK_BOX(ret), dialog_box, TRUE, TRUE, 0);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL(label), 
    _("<b>Edit</b> (Drag widgets into profile / "
      "Use shift+enter to insert a new line)"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_box_pack_start (GTK_BOX(dialog_box), label, FALSE, FALSE, 0);

  /* Widget list */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX(dialog_box), hbox, TRUE, TRUE, 0);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
  get_widget_list (vbox, &sel);

  /* Button bar */
  bbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

  revert_button = gtk_button_new_with_label (_("Revert"));
  gtk_box_pack_start (GTK_BOX(bbox), revert_button, TRUE, TRUE, 0);
  save_button = gtk_button_new_with_label (_("Save profile"));
  gtk_box_pack_start (GTK_BOX(bbox), save_button, TRUE, TRUE, 0);

  edit_window = pidgin_create_imhtml (TRUE, &current_profile, &toolbar, 
                   &sw);
  gtk_box_pack_start (GTK_BOX(hbox), edit_window, TRUE, TRUE, 0);

  /* Finish */
  g_signal_connect (G_OBJECT(save_button), "clicked",
                    G_CALLBACK(save_cb), current_profile);
  g_signal_connect (G_OBJECT(revert_button), "clicked",
                    G_CALLBACK(revert_cb), current_profile); 
  g_signal_connect (G_OBJECT (refresh_button), "clicked",
                    G_CALLBACK (refresh_cb), preview);

  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (current_profile));
  g_signal_connect (G_OBJECT (text_buffer), "changed",
                    G_CALLBACK (event_cb), preview);
  g_signal_connect_after(G_OBJECT(current_profile), "format_function_toggle",
           G_CALLBACK(formatting_toggle_cb), preview);
  g_signal_connect_after(G_OBJECT(current_profile), "format_function_clear",
           G_CALLBACK(formatting_clear_cb), preview); 
 
  revert_cb (revert_button, current_profile); 
  refresh_cb (refresh_button, preview);

  return ret;
}

/*--------------------------------------------------------------------------* 
 * General edit window                                                      *
 *--------------------------------------------------------------------------*/
static void
ap_edit_content_destroy (GtkWidget *button, GtkWidget *window)
{
  if (content_win) {
    gtk_widget_destroy (content_win);
    done_with_widget_list ();
    content_win = NULL; 
    current_profile = NULL;
  }
}

static void ap_edit_content_show ()
{
  GtkWidget *vbox;
  GtkWidget *bbox;
  GtkWidget *notebook;
  GtkWidget *button;

  if (content_win) {
    gtk_window_present (GTK_WINDOW(content_win));
    return;
  }

  /* Create the window */
  content_win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_role (GTK_WINDOW(content_win), "ap_edit_content");
  gtk_window_set_title (GTK_WINDOW(content_win), _("Edit Content"));
  gtk_window_set_default_size (GTK_WINDOW(content_win), 700, 550);
  gtk_container_set_border_width (GTK_CONTAINER(content_win), 6);

  g_signal_connect (G_OBJECT(content_win), "destroy",
    G_CALLBACK(ap_edit_content_destroy), NULL);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER(content_win), vbox);

  /* The notebook */
  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);

  gtk_notebook_append_page (GTK_NOTEBOOK(notebook), 
    ap_widget_get_config_page (), gtk_label_new (_("Widgets")));
  gtk_notebook_append_page (GTK_NOTEBOOK(notebook), get_profile_page (),
    gtk_label_new (_("Info/profile")));

  /* The buttons to press! */
  bbox = gtk_hbutton_box_new ();
  gtk_box_set_spacing (GTK_BOX(bbox), PIDGIN_HIG_BOX_SPACE);
  gtk_button_box_set_layout (GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
  gtk_box_pack_start (GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  g_signal_connect (G_OBJECT(button), "clicked",
    G_CALLBACK(ap_edit_content_destroy), NULL);
  gtk_box_pack_start (GTK_BOX(bbox), button, FALSE, FALSE, 0);

  gtk_widget_show_all (content_win);
}

/*--------------------------------------------------------------------------* 
 * The actions themselves                                                   *
 *--------------------------------------------------------------------------*/
static void edit_content (PurplePluginAction *action)
{
  ap_edit_content_show ();
}

static void edit_preferences (PurplePluginAction *action)
{
  ap_preferences_display ();
}

static void make_visible (PurplePluginAction *action)
{
  ap_gtk_make_visible ();
}

/* Return the actions */
GList *actions (PurplePlugin *plugin, gpointer context)
{
  PurplePluginAction *act;
  GList *list = NULL;

  act = purple_plugin_action_new (_("Edit Content"), edit_content);
  list = g_list_append (list, act); 
  act = purple_plugin_action_new (_("Preferences"), edit_preferences);
  list = g_list_append (list, act);
  act = purple_plugin_action_new (_("Show summary"), make_visible);
  list = g_list_append (list, act);

  return list;
}

void ap_actions_finish () 
{
  if (content_win) ap_edit_content_destroy (NULL, NULL);
  if (accounts_dialog) accounts_response_cb (NULL, 0, NULL);
}


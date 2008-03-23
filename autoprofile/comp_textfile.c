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

/*---------- TEXT FILE: Text from a file ----------*/
static GtkWidget *file_entry;
static GtkWidget *file_selector;

/* Read file into string and return */
char *text_file_generate (struct widget *w)
{
  gchar *text, *salvaged, *converted;
  const gchar *filename;
  int max = ap_prefs_get_int (w, "text_size");

  text = NULL;
  filename = ap_prefs_get_string (w, "text_file");

  if (!g_file_test (filename, G_FILE_TEST_EXISTS)) {
    return strdup (_("[ERROR: File does not exist]"));
  }

  if (!g_file_get_contents (filename, &text, NULL, NULL)) {
    return strdup (_("[ERROR: Unable to open file]"));
  } 

  converted = purple_utf8_try_convert (text);
  if (converted != NULL) {
    g_free (text);
    text = converted;
  }

  if (strlen (text) > max) {
    *(text+max) = '\0';
  }

  salvaged = purple_utf8_salvage (text);
  g_free (text);

  return salvaged;
}

static gboolean text_file_update (GtkWidget *widget, GdkEventFocus *evt, 
                struct widget *w)
{
  ap_prefs_set_string (w, "text_file",
    gtk_entry_get_text (GTK_ENTRY (file_entry)));
  return FALSE;
}

static void text_file_filename (GtkWidget *widget, gpointer user_data) {
  const gchar *selected_filename;
  struct widget *w = (struct widget *) user_data;
 
  selected_filename = gtk_file_selection_get_filename (
    GTK_FILE_SELECTION (file_selector));

  ap_prefs_set_string (w, "text_file", selected_filename);
  gtk_entry_set_text (GTK_ENTRY (file_entry), selected_filename);
}

/* Creates and pops up file selection dialog for fortune file */
static void text_file_selection (GtkWidget *widget, gpointer user_data) {
  const char *cur_file;
  struct widget *w = (struct widget *) user_data;
  
  /* Create the selector */
  file_selector = gtk_file_selection_new (
    "Select a text file with content");

  cur_file = ap_prefs_get_string (w, "text_file");
  if (cur_file && strlen (cur_file) > 1) {
    gtk_file_selection_set_filename (
      GTK_FILE_SELECTION (file_selector), cur_file); 
  }
  
  g_signal_connect (GTK_OBJECT(
                      GTK_FILE_SELECTION(file_selector)->ok_button),
                    "clicked", G_CALLBACK (text_file_filename), w);
   			   
  /* Destroy dialog box when the user clicks button. */
  g_signal_connect_swapped (GTK_OBJECT(
      GTK_FILE_SELECTION(file_selector)->ok_button), 
    "clicked", G_CALLBACK (gtk_widget_destroy), (gpointer) file_selector); 

  g_signal_connect_swapped (GTK_OBJECT (
      GTK_FILE_SELECTION (file_selector)->cancel_button), 
    "clicked", G_CALLBACK (gtk_widget_destroy), (gpointer) file_selector); 
   
  /* Display dialog */
  gtk_widget_show (file_selector);
}

/* Pop up message with instructions */
static void text_file_info_button (GtkButton *button, gpointer data) 
{
  if (!strcmp ((char *) data, "itunes")) {
    purple_notify_formatted (NULL, _("iTunes"), _("Current song in iTunes"), NULL,
      _("Get TuneCam from <a href=\""
        "http://www.soft-o-mat.com/productions.shtml\">"
        "http://www.soft-o-mat.com/productions.shtml</a> and start it.<br>"
        "Create a html file that contains the following text:<br><br>&lt;tc"
        "&gt;artist&lt;/tc&gt; - &lt;tc&gt;title&lt;/tc&gt;<br><br>and "
        "press the \"T\" button.  Import the html file as a template for "
        "the \"File Track\" and whatever else you see fit.  Then select "
        "the \"G\" button and choose the location of the output file which "
        "will be used in this component"), 
      NULL, NULL);  
  } else if (!strcmp ((char *) data, "xmms")) {
    purple_notify_formatted (NULL, _("XMMS"), _("Current song in XMMS"), NULL,
     _("Included in the misc folder of AutoProfile is a script called \""
       "xmms_currenttrack\".  Install this script in your $PATH and give it "
       "executable permissions, and specify the program using a pipe.<br><br>"
       "Alternatively, in XMMS, go to Options->Preferences->Effects/General "
       "Plugins.<br>Configure the \"Song Change\" plugin.  In the song change"
       " command box, put<br><br>echo \"%s\" > /path/to/output/file<br><br>"
       "and be sure to enable the plugin.  Select the file location in "
       "AutoProfile and you should be done"),
      NULL, NULL);
  } else if (!strcmp ((char *) data, "wmp")) {
    purple_notify_formatted (NULL, _("Windows Media Player"), 
      _("Current song in Windows Media Player"), NULL,
      _("Download NowPlaying, a plugin for WMP from <a href=\""
        "http://www.wmplugins.com/ItemDetail.aspx?ItemID=357\">"
        "http://www.wmplugins.com/ItemDetail.aspx?ItemID=357</a> and follow "
        "the included installation instructions.<br>Set the output filename "
        "to the file you choose in this component"),
        NULL, NULL);
  } else if (!strcmp ((char *) data, "amip")) {
    purple_notify_formatted (NULL, _("iTunes/Winamp/Foobar/Apollo/QCD"),
      _("Current song in iTunes/Winamp/Foobar/Apollo/QCD"), NULL,
      _("Get the version of AMIP associated with your player from <a href=\""
        "http://amip.tools-for.net/\">"
        "http://amip.tools-for.net/</a> and install/"
        "enable it.<br>"
        "Check the box \"Write song info to file\", play with the settings, "
        "and set the file in this component to be the file in the AMIP "
        "options."),
      NULL, NULL);
  }
}

/* Create the menu */
GtkWidget *text_file_menu (struct widget *w)
{
  GtkWidget *ret = gtk_vbox_new (FALSE, 5);
  GtkWidget *hbox, *label, *button;

  label = gtk_label_new (_("Select text file with source content"));
  gtk_box_pack_start (GTK_BOX (ret), label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (ret), hbox, FALSE, FALSE, 0);
  /* Text entry to type in file name */
  file_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), file_entry, FALSE, FALSE, 0);
  gtk_entry_set_text (GTK_ENTRY (file_entry), 
                  ap_prefs_get_string (w, "text_file"));
  g_signal_connect (G_OBJECT (file_entry), "focus-out-event",
                    G_CALLBACK (text_file_update), w);
  /* Button to bring up file select dialog */
  button = gtk_button_new_with_label ("Browse ...");
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (text_file_selection), w);

  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

  ap_prefs_labeled_spin_button (w, ret, 
    _("Max characters to read from file:"), "text_size", 
    1, AP_SIZE_MAXIMUM - 1, NULL);
  
  gtk_box_pack_start (GTK_BOX (ret),
    gtk_hseparator_new (), 0, 0, 0);

  /* Windows */
  label = gtk_label_new (_("Windows users: Play the current song in:"));
  gtk_box_pack_start (GTK_BOX (ret), label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (ret), hbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_label ("iTunes/Winamp/Foobar/Apollo/QCD");
  g_signal_connect (G_OBJECT (button), "clicked",
    G_CALLBACK (text_file_info_button), "amip");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0); 
 
  button = gtk_button_new_with_label ("Windows Media Player");
  g_signal_connect (G_OBJECT (button), "clicked",
    G_CALLBACK (text_file_info_button), "wmp");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);  
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (ret), hbox, FALSE, FALSE, 0);
  
  /* *nix */
  label = gtk_label_new (_("*nix users: Play the current song in:"));
  gtk_box_pack_start (GTK_BOX (ret), label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (ret), hbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_label ("XMMS");
  g_signal_connect (G_OBJECT (button), "clicked",
    G_CALLBACK (text_file_info_button), "xmms");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0); 
     
  /* OS X */
  label = gtk_label_new (_("OS X users: Play the current song in:"));
  gtk_box_pack_start (GTK_BOX (ret), label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (ret), hbox, FALSE, FALSE, 0);
 
  button = gtk_button_new_with_label ("iTunes");
  g_signal_connect (G_OBJECT (button), "clicked",
    G_CALLBACK (text_file_info_button), "itunes");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0); 

  return ret;
}

void text_file_init (struct widget *w) {
  ap_prefs_add_string (w, "text_file", "");
  ap_prefs_add_int (w, "text_size", AP_SIZE_MAXIMUM - 1);
}

struct component text =
{
  N_("Text File / Songs"),
  N_("Copies text from file that external programs "
     "(e.g. XMMS, Winamp, iTunes) can modify on a regular basis"),
  "File",
  &text_file_generate,
  &text_file_init,
  NULL,
  NULL,
  NULL,
  &text_file_menu
};


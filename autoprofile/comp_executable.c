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

#include "../common/pp_internal.h"

#include "component.h"
#include "utility.h"

#include <string.h>

/*---------- EXECUTABLE: STDOUT from a program ----------*/
static GtkWidget *file_selector;
static GtkWidget *file_entry;

/* Read file into string and return */
char *executable_generate (struct widget *w)
{
  char *text, *text_start;
  int max;
  gboolean exec;
  GError *return_error;
  
  max = ap_prefs_get_int (w, "max_size");
  exec = g_spawn_command_line_sync (ap_prefs_get_string (w, "command"),
    &text_start, NULL, NULL, &return_error);

  if (!exec) {
    /* Excution failed */
    ap_debug ("executable", "command failed to execute");
    return g_strdup (_("[ERROR: command failed to execute]"));
  }

  if (strlen (text_start) < max)
    text = text_start + strlen(text_start);
  else
    text = text_start + max;

  /* Should back off only if the last item is newline */
  /* Gets rid of the extra <BR> in output */
  text--;
  if (*text != '\n')
    text++;

  *text = '\0';
  return text_start;
}

void executable_filename (GtkWidget *widget, gpointer user_data) {
  const gchar *selected_filename;

  selected_filename = gtk_file_selection_get_filename (
    GTK_FILE_SELECTION (file_selector));

  ap_prefs_set_string ((struct widget *) user_data, "command",
    selected_filename);
  gtk_entry_set_text (GTK_ENTRY (file_entry), selected_filename);
}

/* Creates and pops up file selection dialog for fortune file */
void executable_selection (GtkWidget *widget, struct widget *w) {
  const char *cur_file;
  
  /* Create the selector */
  file_selector = gtk_file_selection_new (
    "Select the location of the program");

  cur_file = ap_prefs_get_string (w, "command");
  if (strlen (cur_file) > 1) {
    gtk_file_selection_set_filename (
      GTK_FILE_SELECTION (file_selector), cur_file); 
  }
  
  g_signal_connect (GTK_OBJECT(
                      GTK_FILE_SELECTION(file_selector)->ok_button),
                    "clicked", G_CALLBACK (executable_filename), w);
   			   
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

static gboolean executable_update (GtkWidget *widget, GdkEventFocus *evt, 
                                gpointer data)
{
  ap_prefs_set_string ((struct widget *) data, "command",
    gtk_entry_get_text (GTK_ENTRY (file_entry)));
  return FALSE;
}

/* Create the menu */
GtkWidget *executable_menu (struct widget *w)
{
  GtkWidget *ret = gtk_vbox_new (FALSE, 5);
  GtkWidget *hbox, *label, *button;

  label = gtk_label_new (
    _("Specify the command line you wish to execute"));
  gtk_box_pack_start (GTK_BOX (ret), label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (ret), hbox, FALSE, FALSE, 0);
  /* Text entry to type in program name */
  file_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), file_entry, FALSE, FALSE, 0);
  gtk_entry_set_text (GTK_ENTRY (file_entry), 
                  ap_prefs_get_string (w, "command"));
  g_signal_connect (G_OBJECT (file_entry), "focus-out-event",
                    G_CALLBACK (executable_update), w);
  /* Button to bring up file select dialog */
  button = gtk_button_new_with_label ("Browse for program");
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (executable_selection), w);

  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

  ap_prefs_labeled_spin_button (w, ret, 
    _("Max characters to read from output: "), "max_size",
    1, AP_SIZE_MAXIMUM, NULL);
  
  return ret;
}

void executable_init (struct widget *w) {
  ap_prefs_add_string (w, "command", "date");
  ap_prefs_add_int (w, "max_size", 1000);
}

struct component executable =
{
  N_("Command Line"),
  N_("Reproduces standard output of running a program on the command line"),
  "Command",
  &executable_generate,
  &executable_init,
  NULL,
  NULL,
  NULL,
  &executable_menu
};


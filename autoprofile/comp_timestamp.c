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

#include "gtkimhtml.h"

/*---------- TIMESTAMP: Display time at creation ---------------------------*/
static char *timestamp_generate (struct widget *w) {
  struct tm *cur_time;
  char *ret;

  time_t *general_time = (time_t *)malloc(sizeof(time_t));
  time (general_time);
  cur_time = ap_localtime (general_time);
  free (general_time);

  ret = (char *)malloc (AP_SIZE_MAXIMUM);
  *ret = '\0';
  
  strftime (ret, AP_SIZE_MAXIMUM - 1, 
    ap_prefs_get_string (w, "timestamp_format"),
    cur_time);

  free (cur_time);
  return ret;
}

static void timestamp_init (struct widget *w) {
  ap_prefs_add_string (w, "timestamp_format",
    "Automatically created at %I:%M %p");
}

static GtkWidget *entry;

static void event_cb (GtkWidget *widget, struct widget *w)
{
  ap_prefs_set_string (w, "timestamp_format", 
                  gtk_imhtml_get_markup (GTK_IMHTML(entry)));
}

static void formatting_toggle_cb (GtkIMHtml *imhtml, 
                GtkIMHtmlButtons buttons, struct widget *w)
{
  ap_prefs_set_string (w, "timestamp_format", 
                  gtk_imhtml_get_markup (GTK_IMHTML(entry)));

}

static void formatting_clear_cb (GtkIMHtml *imhtml,
               struct widget *w)
{
  ap_prefs_set_string (w, "timestamp_format", 
                  gtk_imhtml_get_markup (GTK_IMHTML(entry)));
}


static GtkWidget *timestamp_menu (struct widget *w)
{
  GtkWidget *ret = gtk_vbox_new (FALSE, 5);
  GtkWidget *label, *sw;

  GtkWidget *entry_window, *toolbar;
  GtkTextBuffer *text_buffer;
  
  entry_window = pidgin_create_imhtml (TRUE, &entry, &toolbar, &sw);
  gtk_box_pack_start (GTK_BOX (ret), entry_window, FALSE, FALSE, 0);            gtk_imhtml_append_text_with_images (GTK_IMHTML(entry),
                  ap_prefs_get_string (w, "timestamp_format"),
                  0, NULL);
  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (entry));
  g_signal_connect (G_OBJECT (text_buffer), "changed",
                    G_CALLBACK (event_cb), w);
  g_signal_connect_after(G_OBJECT(entry), "format_function_toggle",
           G_CALLBACK(formatting_toggle_cb), w);
  g_signal_connect_after(G_OBJECT(entry), "format_function_clear",
           G_CALLBACK(formatting_clear_cb), w);

  label = gtk_label_new (_(
    "Insert the following characters where time is to be displayed:\n\n"
    "%H\thour (24-hour clock)\n"
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
    "%%\t%" ));
  sw = gtk_scrolled_window_new (NULL,NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (ret), sw, TRUE, TRUE , 0);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(sw), label);

  return ret;
}

struct component timestamp =
{
  N_("Timestamp"),
  N_("Displays custom text showing when message was created"),
  "Timestamp",
  &timestamp_generate,
  &timestamp_init,
  NULL,
  NULL,
  NULL,
  &timestamp_menu
};


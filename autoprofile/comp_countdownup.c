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
#include "gtkprefs.h"
#include "utility.h"

#include <math.h>

static GtkWidget *spin_secs;
static GtkWidget *spin_mins;
static GtkWidget *spin_hour;
static GtkWidget *spin_day;
static GtkWidget *spin_month;
static GtkWidget *spin_year;

/* Generate the time! */
char *count_generate (struct widget *w) 
{
  double d_secs, d_mins, d_hours, d_days;
  char *s_secs, *s_mins, *s_hours, *s_days;
  double difference;
  int l, s;

  struct tm *ref_time;
  char *result;

  ref_time = (struct tm *) malloc (sizeof (struct tm));

  ref_time->tm_sec = ap_prefs_get_int (w, "secs");
  ref_time->tm_min = ap_prefs_get_int (w, "mins");
  ref_time->tm_hour = ap_prefs_get_int (w, "hour");
  ref_time->tm_mday = ap_prefs_get_int (w, "day");
  ref_time->tm_mon = ap_prefs_get_int (w, "month") - 1;
  ref_time->tm_year = ap_prefs_get_int (w, "year") - 1900;
  ref_time->tm_isdst = -1;

  mktime (ref_time);

  if (ap_prefs_get_int (w, "down") == 1)
    difference = difftime (mktime (ref_time), time(NULL));
  else
    difference = difftime (time(NULL), mktime (ref_time));

  if (difference < 0) {
    d_secs = 0;
    d_mins = 0;
    d_hours = 0;
    d_days = 0;
  } else {
    d_mins = floor (difference / 60);
    d_secs = difference - (d_mins * 60);
    d_hours = floor (d_mins / 60);
    d_mins = d_mins - (d_hours * 60);
    d_days = floor (d_hours / 24);
    d_hours = d_hours - (d_days * 24);
  }

  result = (char *)malloc(sizeof (char) * AP_SIZE_MAXIMUM);
  l = ap_prefs_get_int (w, "large");
  s = ap_prefs_get_int (w, "small");

  if (l < s) {
   g_snprintf(result, AP_SIZE_MAXIMUM, 
      "%.0f days, %.0f hours, %.0f minutes, %.0f seconds",
      d_days, d_hours, d_mins, d_secs); 
   free (ref_time);
   return result;
  }

  if (l < 3)
    d_hours = d_hours + (d_days * 24);
  if (l < 2)
    d_mins = d_mins + (d_hours * 60);
  if (l < 1)
    d_secs = d_secs + (d_mins * 60);

  if (d_days == 1.0)
    s_days = g_strdup ("day");
  else
    s_days = g_strdup ("days");

  if (d_hours == 1.0)
    s_hours = g_strdup ("hour");
  else
    s_hours = g_strdup ("hours");

  if (d_mins == 1.0)
    s_mins = g_strdup ("minute");
  else
    s_mins = g_strdup ("minutes");

  if (d_secs == 1.0)
    s_secs = g_strdup ("second");
  else
    s_secs = g_strdup ("seconds");

  switch (l) {
    case 3:
      switch (s) {
        case 3:
          g_snprintf (result, AP_SIZE_MAXIMUM, "%.0f %s",
            d_days, s_days);
          break;
        case 2:
          g_snprintf (result, AP_SIZE_MAXIMUM, "%.0f %s, %.0f %s",
            d_days, s_days, d_hours, s_hours);
          break;
        case 1:
          g_snprintf (result, AP_SIZE_MAXIMUM, "%.0f %s, %.0f %s, %.0f %s",
            d_days, s_days, d_hours, s_hours, d_mins, s_mins);
          break;
        case 0:
          g_snprintf (result, AP_SIZE_MAXIMUM, 
            "%.0f %s, %.0f %s, %.0f %s, %.0f %s",
            d_days, s_days, d_hours, s_hours, d_mins, s_mins, d_secs, s_secs);
          break;
        default:
          *result = '\0';
      }
      break;
    case 2:
      switch (s) {
        case 2:
          g_snprintf (result, AP_SIZE_MAXIMUM, "%.0f %s",
            d_hours, s_hours);
          break;
        case 1:
          g_snprintf (result, AP_SIZE_MAXIMUM, "%.0f %s, %.0f %s",
            d_hours, s_hours, d_mins, s_mins);
          break;
        case 0:
          g_snprintf (result, AP_SIZE_MAXIMUM, "%.0f %s, %.0f %s, %.0f %s",
            d_hours, s_hours, d_mins, s_mins, d_secs, s_secs);
          break;
        default:
          *result = '\0';
      }
      break;
    case 1:
      switch (s) {
        case 1:
          g_snprintf (result, AP_SIZE_MAXIMUM, "%.0f %s",
            d_mins, s_mins);
          break;
        case 0:
          g_snprintf (result, AP_SIZE_MAXIMUM, "%.0f %s, %.0f %s",
            d_mins, s_mins, d_secs, s_secs);
          break;
        default:
          *result = '\0';
      }
      break;
    case 0:
      g_snprintf (result, AP_SIZE_MAXIMUM, "%.0f %s",
        d_secs, s_secs);
      break;
    default:
      *result = '\0';
  }

  free (s_days);
  free (s_hours);
  free (s_mins);
  free (s_secs);
  free (ref_time);

  return result;
}

static void update_year (GtkSpinButton *spinner, struct widget *w)
{
  int value = gtk_spin_button_get_value_as_int (spinner);
  ap_prefs_set_int (w, "year", value);
}

static void update_month (GtkSpinButton *spinner, struct widget *w)
{
  int value = gtk_spin_button_get_value_as_int (spinner);
  ap_prefs_set_int (w, "month", value);
}

static void update_day (GtkSpinButton *spinner, struct widget *w)
{
  int value = gtk_spin_button_get_value_as_int (spinner);
  ap_prefs_set_int (w, "day", value);
}

static void update_hour (GtkSpinButton *spinner, struct widget *w)
{
  int value = gtk_spin_button_get_value_as_int (spinner);
  ap_prefs_set_int (w, "hour", value);
}

static void update_mins (GtkSpinButton *spinner, struct widget *w)
{
  int value = gtk_spin_button_get_value_as_int (spinner);
  ap_prefs_set_int (w, "mins", value);
}

static void update_secs (GtkSpinButton *spinner, struct widget *w)
{
  int value = gtk_spin_button_get_value_as_int (spinner);
  ap_prefs_set_int (w, "secs", value);
}

static void set_to_current_time (GtkButton *button, struct widget *w)
{
  time_t the_time;
  struct tm *ref_time;

  the_time = time(NULL);
  ref_time = ap_localtime(&the_time);
  ap_prefs_set_int (w, "year",  ref_time->tm_year + 1900);
  ap_prefs_set_int (w, "month", ref_time->tm_mon + 1);
  ap_prefs_set_int (w, "day",   ref_time->tm_mday);
  ap_prefs_set_int (w, "hour",  ref_time->tm_hour);
  ap_prefs_set_int (w, "mins",  ref_time->tm_min);
  ap_prefs_set_int (w, "secs",  ref_time->tm_sec);
  free (ref_time);

  if (spin_secs != NULL) {
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_secs), 
      ap_prefs_get_int (w, "secs"));
  }
  if (spin_mins != NULL) {
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_mins), 
      ap_prefs_get_int (w, "mins"));
  } 
  if (spin_hour != NULL) {
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_hour), 
      ap_prefs_get_int (w, "hour"));
  }
  if (spin_day != NULL) {
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_day), 
      ap_prefs_get_int (w, "day"));
  }
  if (spin_month != NULL) {
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_month), 
      ap_prefs_get_int (w, "month"));
  }
  if (spin_year != NULL) {
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_year), 
      ap_prefs_get_int (w, "year"));
  }
}

GtkWidget *count_menu (struct widget *w)
{
  GtkWidget *vbox, *hbox, *big_hbox, *frame;
  GtkWidget *label, *spinner, *dropbox, *button;
  GList *options;

  big_hbox = gtk_hbox_new (FALSE, 6);
  
  frame = pidgin_make_frame (big_hbox, _("Start/end time"));
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Year: "));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_box_pack_start (GTK_BOX(hbox), label, TRUE, TRUE, 0);
  spinner = gtk_spin_button_new_with_range (1970, 2035, 1);
  gtk_box_pack_end (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinner), 
    ap_prefs_get_int (w, "year"));
  g_signal_connect (G_OBJECT (spinner), "value-changed",
                    G_CALLBACK (update_year), w);
  spin_year = spinner;

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Month: "));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_box_pack_start (GTK_BOX(hbox), label, TRUE, TRUE, 0);
  spinner = gtk_spin_button_new_with_range (1, 12, 1);
  gtk_box_pack_end (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinner), 
    ap_prefs_get_int (w, "month"));
  g_signal_connect (G_OBJECT (spinner), "value-changed",
                    G_CALLBACK (update_month), w);
  spin_month = spinner;

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Day: "));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_box_pack_start (GTK_BOX(hbox), label, TRUE, TRUE, 0);
  spinner = gtk_spin_button_new_with_range (1, 31, 1);
  gtk_box_pack_end (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinner), 
    ap_prefs_get_int (w, "day"));
  g_signal_connect (G_OBJECT (spinner), "value-changed",
                    G_CALLBACK (update_day), w);
  spin_day = spinner;

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Hour: "));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_box_pack_start (GTK_BOX(hbox), label, TRUE, TRUE, 0);
  spinner = gtk_spin_button_new_with_range (0, 23, 1);
  gtk_box_pack_end (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinner), 
    ap_prefs_get_int (w, "hour"));
  g_signal_connect (G_OBJECT (spinner), "value-changed",
                    G_CALLBACK (update_hour), w);
  spin_hour = spinner;

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Minutes: "));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_box_pack_start (GTK_BOX(hbox), label, TRUE, TRUE, 0);
  spinner = gtk_spin_button_new_with_range (0, 59, 1);
  gtk_box_pack_end (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinner), 
    ap_prefs_get_int (w, "mins"));
  g_signal_connect (G_OBJECT (spinner), "value-changed",
                    G_CALLBACK (update_mins), w);
  spin_mins = spinner;

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Seconds: "));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_box_pack_start (GTK_BOX(hbox), label, TRUE, TRUE, 0);
  spinner = gtk_spin_button_new_with_range (0, 59, 1);
  gtk_box_pack_end (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinner), 
    ap_prefs_get_int (w, "secs"));
  g_signal_connect (G_OBJECT (spinner), "value-changed",
                    G_CALLBACK (update_secs), w);
  spin_secs = spinner;

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  button = gtk_button_new_with_label ("Set to current time");

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (set_to_current_time), w);
  gtk_box_pack_start (GTK_BOX(hbox), button, FALSE, FALSE, 0);

  frame = pidgin_make_frame (big_hbox, _("Which way"));
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  options = g_list_append (NULL,    (char *) _("Count down to stop date"));
  options = g_list_append (options, GINT_TO_POINTER(1));
  options = g_list_append (options, (char *)
                                      _("Count time since start date"));
  options = g_list_append (options, GINT_TO_POINTER(0));

  dropbox = ap_prefs_dropdown_from_list (w, vbox, NULL,
    PURPLE_PREF_INT, "down", options);
  g_list_free (options);

  options = g_list_append (NULL,    (char *) _("Days"));
  options = g_list_append (options, GINT_TO_POINTER(3));
  options = g_list_append (options, (char *) _("Hours"));
  options = g_list_append (options, GINT_TO_POINTER(2));
  options = g_list_append (options, (char *) _("Minutes"));
  options = g_list_append (options, GINT_TO_POINTER(1));
  options = g_list_append (options, (char *) _("Seconds"));
  options = g_list_append (options, GINT_TO_POINTER(0));

  dropbox = ap_prefs_dropdown_from_list (w, vbox, 
    _("Largest units displayed"), PURPLE_PREF_INT, "large", options);
  dropbox = ap_prefs_dropdown_from_list (w, vbox, 
    _("Smallest units displayed"), PURPLE_PREF_INT, "small", options);
  g_list_free (options);
  
  return big_hbox;
}

/* Init prefs */
void count_init (struct widget *w) {
  time_t the_time;
  struct tm *ref_time;

  the_time = time(NULL);
  ref_time = ap_localtime(&the_time);

  ap_prefs_add_int (w, "down", 1);
  ap_prefs_add_int (w, "small", 0);
  ap_prefs_add_int (w, "large", 3);
  ap_prefs_add_int (w, "year",
    ref_time->tm_year + 1900);
  ap_prefs_add_int (w, "month",
    ref_time->tm_mon + 1);
  ap_prefs_add_int (w, "day",
    ref_time->tm_mday);
  ap_prefs_add_int (w, "hour", 
    ref_time->tm_hour);
  ap_prefs_add_int (w, "mins",
    ref_time->tm_min);
  ap_prefs_add_int (w, "secs",
    ref_time->tm_sec);
  free (ref_time);
}

struct component count =
{
  N_("Countdown timer"),
  N_("Given a date, shows amount of time until it (or since it)"),
  "Timer",
  &count_generate,
  &count_init,
  NULL,
  NULL,
  NULL,
  &count_menu
};


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
#include "idle.h"
#include "conversation.h"

#define AP_IDLE_CHECK_INTERVAL 5

static guint check_timeout = 0;
static guint pref_cb = 0;
static time_t last_active_time = 0;

static gboolean is_idle ()
{
  PurpleIdleUiOps *ui_ops;
  time_t time_idle;
  const gchar *idle_reporting;

  ui_ops = purple_idle_get_ui_ops ();

  idle_reporting = purple_prefs_get_string ("/purple/away/idle_reporting");
  if (!strcmp (idle_reporting, "system") &&
      (ui_ops != NULL) && (ui_ops->get_time_idle != NULL)) {
    time_idle = time (NULL) - last_active_time;
  } else if (!strcmp (idle_reporting, "gaim")) {
    time_idle = time (NULL) - last_active_time;
  } else {
    time_idle = 0;
  }

  return (time_idle > 
           (60 * purple_prefs_get_int("/purple/away/mins_before_away")));
}

static gboolean ap_check_idleness (gpointer data) 
{
  gboolean auto_away;

  // ap auto idle
  // 0    0    0  don't do anything
  // 0    0    1  ap_use_idleaway ()
  // 1    0    x  don't do anything, we're already away
  // 1    1    0  ap_dont_use_idleaway ()
  // 1    1    1  don't do anything

  if (ap_is_currently_away () && !ap_autoaway_in_use ()) return TRUE;
  auto_away = purple_prefs_get_bool (
    "/plugins/gtk/autoprofile/away_when_idle");

  if (is_idle ()) {
    if (auto_away && !ap_is_currently_away () && !ap_autoaway_in_use ()) {
      ap_autoaway_enable ();
    }
  } else {
    if (ap_is_currently_away () && ap_autoaway_in_use ()) {
      ap_autoaway_disable ();
    }
  } 
 
  return TRUE;
}

void ap_autoaway_touch ()
{
  time (&last_active_time);
}


static gboolean writing_im_msg_cb (PurpleAccount *account, const char *who,
  char **message, PurpleConversation *conv, PurpleMessageFlags flags) 
{
  ap_autoaway_touch ();
  ap_check_idleness (NULL);
  return FALSE;
}

static void auto_pref_cb (
  const char *name, PurplePrefType type, gconstpointer val, gpointer data) 
{
  if (!purple_prefs_get_bool ("/purple/away/away_when_idle")) return;

  purple_notify_error (NULL, NULL,
    N_("This preference is disabled"), 
    N_("This preference currently has no effect because AutoProfile is in "
       "use.  To modify this behavior, use the AutoProfile configuration "
       "menu."));

  purple_prefs_set_bool ("/purple/away/away_when_idle", FALSE);
}

/*--------------------------------------------------------------------------*
 * Global functions to start it all                                         *
 *--------------------------------------------------------------------------*/
void ap_autoaway_start ()
{
  purple_prefs_set_bool ("/purple/away/away_when_idle", FALSE);

  check_timeout = purple_timeout_add (AP_IDLE_CHECK_INTERVAL * 1000,
    ap_check_idleness, NULL);
  
  purple_signal_connect (purple_conversations_get_handle (), "writing-im-msg",
    ap_get_plugin_handle (), PURPLE_CALLBACK(writing_im_msg_cb), NULL);

  pref_cb = purple_prefs_connect_callback (ap_get_plugin_handle (),
    "/purple/away/away_when_idle", auto_pref_cb, NULL);

  ap_autoaway_touch ();
}

void ap_autoaway_finish ()
{
  // Assumes signals are disconnected globally
        
  purple_prefs_disconnect_callback (pref_cb);
  pref_cb = 0;

  if (check_timeout > 0) purple_timeout_remove (check_timeout);
  check_timeout = 0;

  purple_prefs_set_bool ("/purple/away/away_when_idle",
          purple_prefs_get_bool ("/plugins/gtk/autoprofile/away_when_idle"));
}



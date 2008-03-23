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

#ifndef AUTOPROFILE_H
#define AUTOPROFILE_H

#include "../common/pp_internal.h"

#include "sizes.h"
#include "widget.h"
#include "utility.h"

#include "plugin.h"
#include "gtkplugin.h"

#include "signals.h"
#include "prefs.h"
#include "util.h"
#include "notify.h"

#include "string.h"
#include "time.h"

#define AP_SCHEDULE_UPDATE_DELAY 3000
#define AP_GTK_MAX_MESSAGES 50

/* Data types */
typedef enum
{
  AP_MESSAGE_TYPE_OTHER = -1,
  AP_MESSAGE_TYPE_PROFILE,
  AP_MESSAGE_TYPE_AWAY,
  AP_MESSAGE_TYPE_AVAILABLE,
  AP_MESSAGE_TYPE_STATUS
} APMessageType;

typedef enum 
{
  AP_UPDATE_UNKNOWN = 0,
  AP_UPDATE_STATUS,
  AP_UPDATE_PROFILE
} APUpdateType;

/* Variable access functions */
PurplePlugin *ap_get_plugin_handle ();
gboolean ap_is_currently_away ();

void ap_account_enable_profile (const PurpleAccount *, gboolean);
gboolean ap_account_has_profile_enabled (const PurpleAccount *);

/* Core behavior functions */
gchar *ap_generate (const char *, gint);
gchar *ap_get_sample_status_message (PurpleAccount *account);
void ap_update (APUpdateType);
void ap_update_after_delay (APUpdateType);
void ap_update_stop (APUpdateType);

/* Queueing functions */
void ap_update_queueing ();

/* Auto-away functions */
void ap_autoaway_start ();
void ap_autoaway_finish ();
void ap_autoaway_touch ();
void ap_autoaway_enable ();
void ap_autoaway_disable ();
gboolean ap_autoaway_in_use ();

/* Auto-reply functions */
void ap_autoreply_start ();
void ap_autoreply_finish ();

/* Gtk Away Messages */
void ap_gtk_start ();
void ap_gtk_finish ();
void ap_gtk_make_visible ();
void ap_gtk_add_message (APUpdateType, APMessageType, const gchar *);
void ap_gtk_set_progress_visible (APUpdateType, gboolean);

/* Gtk Actions */
GList *actions (PurplePlugin *, gpointer);
void ap_actions_finish ();

/* Preferences */
PidginPluginUiInfo ui_info;
void ap_preferences_display ();
void ap_gtk_prefs_add_summary_option (GtkWidget *);
GtkWidget *get_account_page ();

#endif /* #ifndef AUTOPROFILE_H */

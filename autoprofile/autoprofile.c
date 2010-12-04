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

#include "version.h"
#include "savedstatuses.h"

/* General functions */
static void ap_status_changed (
  const char *, PurplePrefType, gconstpointer, gpointer);
static void ap_account_connected (PurpleConnection *);

static void ap_delete_legacy_prefs ();

static void ap_update_queue_start ();
static void ap_update_queue_finish ();

/*--------------------------------------------------------------------------
 * GENERAL VARIABLES
 *------------------------------------------------------------------------*/

static PurplePlugin *plugin_handle = NULL;
static PurpleSavedStatus *current_ap_status = NULL;

static GStaticMutex update_timeout_mutex = G_STATIC_MUTEX_INIT;
static GHashTable *update_timeouts = NULL;

static gboolean using_idleaway = FALSE;

static GStaticMutex update_queue_mutex = G_STATIC_MUTEX_INIT;
static GList *queued_profiles = NULL;
static guint update_queue_timeout = 0;

/* Functions related to general variables */
PurplePlugin *ap_get_plugin_handle () { return plugin_handle; }

gboolean ap_is_currently_away () { 
  return current_ap_status != NULL && 
         purple_savedstatus_get_type (current_ap_status) == PURPLE_STATUS_AWAY;
}

/*--------------------------------------------------------------------------
 * REQUIRED GAIM FUNCTIONS- INFO, INITIALIZATION, UNLOADING
 *------------------------------------------------------------------------*/
/* What to do when plugin is loaded */
static gboolean plugin_load (PurplePlugin *plugin)
{
  GList *accounts_pref;
        
  ap_debug ("general", "AutoProfile is being loaded");

  plugin_handle = plugin;
  current_ap_status = purple_savedstatus_new (NULL, PURPLE_STATUS_UNSET);
  update_timeouts = g_hash_table_new (NULL, NULL);

  ap_delete_legacy_prefs ();

  /* The core autoprofile tracking system */
  purple_prefs_connect_callback (plugin_handle, "/purple/savedstatus/current",
    ap_status_changed, NULL);
  purple_signal_connect (purple_connections_get_handle (),
                       "signed-on", plugin_handle,
                       PURPLE_CALLBACK (ap_account_connected), NULL);

  ap_component_start (); 
  ap_gtk_start ();

  accounts_pref = purple_prefs_get_string_list (
    "/plugins/gtk/autoprofile/profile_accounts");
  ap_gtk_set_progress_visible (AP_UPDATE_PROFILE, (accounts_pref != NULL));
  free_string_list (accounts_pref);
  
  ap_update_after_delay (AP_UPDATE_STATUS);
  ap_update_after_delay (AP_UPDATE_PROFILE);

  ap_autoaway_start ();
  ap_autoreply_start ();

  ap_update_queue_start ();

  return TRUE;
}

/* What to do when plugin is unloaded */
static gboolean plugin_unload (PurplePlugin *plugin)
{
  ap_update_queue_finish ();
  
  ap_autoreply_finish ();
  ap_autoaway_finish (); 

  /* General vars */
  using_idleaway = FALSE;

  ap_update_stop (AP_UPDATE_STATUS);
  ap_update_stop (AP_UPDATE_PROFILE);

  /* Disconnect tracking system */
  purple_signals_disconnect_by_handle (plugin);

  ap_actions_finish ();
  ap_gtk_finish ();
  ap_component_finish ();

  g_hash_table_destroy (update_timeouts);
  return TRUE;
}

/* General information */
static PurplePluginInfo info =
{
  PURPLE_PLUGIN_MAGIC,
  PURPLE_MAJOR_VERSION,
  PURPLE_MINOR_VERSION,
  PURPLE_PLUGIN_STANDARD,                                 /* type           */
  PIDGIN_PLUGIN_TYPE,                                     /* ui_requirement */
  0,                                                      /* flags          */
  NULL,                                                   /* dependencies   */
  PURPLE_PRIORITY_DEFAULT,                                /* priority       */

  N_("gtk-kluge-autoprofile"),                            /* id             */
  N_("AutoProfile"),                                      /* name           */
  PP_VERSION,                                             /* version        */
  N_("User profile and status message content generator"),/* summary        */
                                                          /* description    */
  N_("Allows user to place dynamic text into profiles\n"
     "and status messages, with the text automatically\n"
     "updated whenever content changes"),
                                                          /* author         */
  N_("Casey Ho <casey at hkn-berkeley-edu>"
     "\n\t\t\taim:caseyho"),
  PP_WEBSITE,                                             /* homepage       */
  plugin_load,                                            /* load           */
  plugin_unload,                                          /* unload         */
  NULL,                                                   /* destroy        */

  &ui_info,                                               /* ui_info        */
  NULL,                                                   /* extra_info     */
  NULL,                                                   /* prefs_info     */
  actions,                                                /* actions        */
  NULL,
  NULL,
  NULL,
  NULL
};

/*--------------------------------------------------------------------------
 * CORE FUNCTIONS                                          
 *------------------------------------------------------------------------*/

static gint get_max_size_status (
  const PurpleAccount *account, const PurpleStatusPrimitive type) {
  const char *id;

  if (account == NULL) {
    switch (type) {
      case PURPLE_STATUS_AVAILABLE:             return AP_SIZE_AVAILABLE_MAX;
      case PURPLE_STATUS_AWAY:                  return AP_SIZE_AWAY_MAX;
      default:                                return AP_SIZE_MAXIMUM;
    }
  } else {
    id = purple_account_get_protocol_id (account);

    switch (type) { 
      case PURPLE_STATUS_AVAILABLE:
        if (!strcmp (id, "prpl-oscar"))       return AP_SIZE_AVAILABLE_AIM;
        else if (!strcmp (id, "prpl-msn"))    return AP_SIZE_AVAILABLE_MSN;
        else if (!strcmp (id, "prpl-yahoo"))  return AP_SIZE_AVAILABLE_YAHOO;
        else                                  return AP_SIZE_AVAILABLE_MAX; 
      case PURPLE_STATUS_AWAY:
        if (!strcmp (id, "prpl-oscar"))       return AP_SIZE_AWAY_AIM;
        else                                  return AP_SIZE_AWAY_MAX; 
      default:
                                              return AP_SIZE_MAXIMUM;
    }
  }
}

static const char *ap_savedstatus_get_message (
                const PurpleSavedStatus *status, const PurpleAccount *account)
{
  const PurpleSavedStatusSub *substatus;
        
  substatus = purple_savedstatus_get_substatus(status, account);
  if (substatus != NULL) {
    return purple_savedstatus_substatus_get_message (substatus);
  } 
  return purple_savedstatus_get_message (status);
}

static PurpleStatusPrimitive ap_savedstatus_get_type (
                const PurpleSavedStatus *status, const PurpleAccount *account)
{
  const PurpleSavedStatusSub *substatus;

  substatus = purple_savedstatus_get_substatus(status, account);
  if (substatus != NULL) {
    return purple_status_type_get_primitive (
                      purple_savedstatus_substatus_get_type (substatus));
  } 
  return purple_savedstatus_get_type (status);
}

gchar *ap_get_sample_status_message (PurpleAccount *account) 
{
  const PurpleSavedStatus *s;
  const gchar *message;
  PurpleStatusPrimitive type;

  s = (using_idleaway? purple_savedstatus_get_idleaway () :
                       purple_savedstatus_get_current ()); 
  message = ap_savedstatus_get_message (s, account);
  type = ap_savedstatus_get_type (s, account);
  
  if (!message) return NULL;
  return ap_generate (message, get_max_size_status (account, type));
}

/* Generator helper */
static gchar *ap_process_replacement (const gchar *f) {
  GString *s;
  struct widget *w;
  gchar *result;

  w = ap_widget_find (f);
  if (w) {
    result = w->component->generate (w);
    return result;
  } else {
    s = g_string_new ("");
    g_string_printf (s, "[%s]", f);
    result = s->str;
    g_string_free (s, FALSE);
    return result;
  }
}

/* The workhorse generation function! */
gchar *ap_generate (const gchar *f, gint max_length) {
  GString *output;
  gchar *result;
  gchar *format, *format_start, *percent_start;
  gchar *replacement;
  int state;

  output = g_string_new ("");
  format_start = format = purple_utf8_salvage (f);

  /* When a % has been read (and searching for next %), state is 1
   * otherwise it is 0
   */
  state = 0;
  percent_start = NULL;

  while (*format) {
    if (state == 1) {
      if (*format == '[') {
        g_string_append_unichar (output, g_utf8_get_char ("["));
        *format = '\0';
        g_string_append (output, percent_start);
        percent_start = format = format+1;        
      } else if (*format == ']') {
        *format = '\0';
        format++;
        state = 0;
        replacement = ap_process_replacement (percent_start);
        percent_start = NULL;
        g_string_append (output, replacement);
        free (replacement);
      } else {
        format = g_utf8_next_char (format);
      }
    } else {
      if (*format == '\n') {
        g_string_append (output, "<br>");
      } else if (*format == '[') {
        state = 1;
        percent_start = format+1;
      } else {
        g_string_append_unichar (output, g_utf8_get_char (format));
      }
      format = g_utf8_next_char (format);
    }
  }

  /* Deal with case where final ] not found */
  if (state == 1) {
    g_string_append_unichar (output, g_utf8_get_char ("["));
    g_string_append (output, percent_start);
  }

  /* Set size limit */
  g_string_truncate (output, max_length);

  /* Finish up */
  free (format_start);
  result = purple_utf8_salvage(output->str);
  g_string_free (output, TRUE);

  return result;
}

void ap_account_enable_profile (const PurpleAccount *account, gboolean enable) {
  GList *original, *new;
  gboolean original_status;

  gchar *username, *protocol_id;

  GList *node, *tmp;
  GList *ret = NULL;

  original_status = ap_account_has_profile_enabled (account);
  if (original_status == enable) {
    ap_debug_warn ("profile", "New status identical to original, skipping");
    return;
  }

  original = purple_prefs_get_string_list (
    "/plugins/gtk/autoprofile/profile_accounts");
  username = strdup (purple_account_get_username (account));
  protocol_id = strdup (purple_account_get_protocol_id (account));

  if (!enable) {
    /* Remove from the list */
    ap_debug ("profile", "Disabling profile updates for account");

    while (original) {
      if (!strcmp (original->data, username) &&
          !strcmp (original->next->data, protocol_id)) {
        node = original;
        tmp = node->next;
        original = original->next->next;
        free (node->data);
        free (tmp->data);
        g_list_free_1 (node);
        g_list_free_1 (tmp);
        free (username);
        free (protocol_id);
      } else {
        node = original;
        original = original->next->next;
        node->next->next = ret;
        ret = node;
      }
    }

    new = ret;
  } else {
    /* Otherwise add on */
    GList *ret_start, *ret_end;
   
    ap_debug ("profile", "enabling profile updates for account"); 
  
    ret_start = (GList *) malloc (sizeof (GList));
    ret_end = (GList *) malloc (sizeof (GList));
    ret_start->data = username;
    ret_start->next = ret_end;
    ret_end->data = protocol_id;
    ret_end->next = original;

    new = ret_start;
  }

  purple_prefs_set_string_list (
    "/plugins/gtk/autoprofile/profile_accounts", new);
  ap_gtk_set_progress_visible (AP_UPDATE_PROFILE, (new != NULL));
  
  free_string_list (new);
}

gboolean ap_account_has_profile_enabled (const PurpleAccount *account) {
  GList *accounts_list, *start_list;

  accounts_list = purple_prefs_get_string_list (
      "/plugins/gtk/autoprofile/profile_accounts");

  start_list = accounts_list;

  /* Search through list of values */
  while (accounts_list) {
    // Make sure these things come in pairs
    if (accounts_list->next == NULL) {
      ap_debug_error ("is_account_profile_enabled", "invalid account string");
      free_string_list (start_list);
      return FALSE;
    }

    // Check usernames
    if (!strcmp ((char *) accounts_list->data, account->username)) {
      // Check protocol
      if (!strcmp ((char *) accounts_list->next->data, account->protocol_id))
      {
        free_string_list (start_list);
        return TRUE;
      }
    } 

    accounts_list = accounts_list->next->next;
  }

  /* Not found, hence it wasn't enabled */
  free_string_list (start_list);
  return FALSE;
}

/* Profiles: Update every so often */
static gboolean ap_update_profile () {
  PurpleAccount *account;
  const GList *purple_accounts;
  gboolean account_updated;
  
  const char *format; 
  const char *old_info;
  char *generated_profile;

  /* Generate the profile text */
  format = purple_prefs_get_string ("/plugins/gtk/autoprofile/profile");

  if (format == NULL) {
    ap_debug_error ("general", "profile is null");
    return FALSE;
  }

  generated_profile = ap_generate (format, AP_SIZE_PROFILE_MAX);

  // If string is blank, nothing would happen
  if (*generated_profile == '\0') {
    free (generated_profile);
    ap_debug_misc ("general", "empty profile set");
    generated_profile = strdup (" ");
  }

  /* Get all accounts and search through each */
  account_updated = FALSE;
  for (purple_accounts = purple_accounts_get_all ();
       purple_accounts != NULL;
       purple_accounts = purple_accounts->next) {
    account = (PurpleAccount *)purple_accounts->data;
    old_info = purple_account_get_user_info (account);
    
    /* Check to see if update option set on account */
    if (ap_account_has_profile_enabled (account) &&
        (old_info == NULL || strcmp (old_info, generated_profile))) {
      purple_account_set_user_info (account, generated_profile);
      account_updated = TRUE;
      
      if (purple_account_is_connected (account)) {
        g_static_mutex_lock (&update_queue_mutex);
        if (g_list_find (queued_profiles, account) == NULL) {
          queued_profiles = g_list_append (queued_profiles, account);
        }
        g_static_mutex_unlock (&update_queue_mutex);
      } else {
        ap_debug_misc ("general", "account not online, not setting profile");
      }
    } 
  }

  if (account_updated) {
    ap_gtk_add_message (AP_UPDATE_PROFILE, AP_MESSAGE_TYPE_PROFILE, 
                        generated_profile);
  }
  
  free (generated_profile);
  return account_updated;
}

static gboolean ap_update_status () 
{
  const PurpleSavedStatus *template_status;  
  GHashTable *substatus_messages;
  gchar *new_message, *new_substatus_message;
  const gchar *sample_message, *old_message;
  const GList *accounts;
  gboolean updated;
  PurpleStatusPrimitive old_type, new_type;
  const PurpleStatusType *substatus_type;
  PurpleAccount *account;
  PurpleSavedStatusSub *substatus;

  template_status = (using_idleaway? purple_savedstatus_get_idleaway () :
                                     purple_savedstatus_get_current ());
  updated = FALSE;

  /* If there are substatuses */
  if (purple_savedstatus_has_substatuses (template_status)) {
    substatus_messages = g_hash_table_new (NULL, NULL);
    for (accounts = purple_accounts_get_all (); 
         accounts != NULL;
         accounts = accounts->next) 
    {
      account = (PurpleAccount *) accounts->data;

      substatus = purple_savedstatus_get_substatus (template_status, account);
      if (substatus) {
        new_type = purple_status_type_get_primitive (
          purple_savedstatus_substatus_get_type (substatus));
        sample_message = 
          purple_savedstatus_substatus_get_message (substatus);

        if (sample_message) {
          new_substatus_message = ap_generate (sample_message, 
            get_max_size_status (account, new_type));
        } else {
          new_substatus_message = NULL;
        }

        g_hash_table_insert (substatus_messages, account, 
          new_substatus_message);

        if (!updated) {
          old_type = ap_savedstatus_get_type (current_ap_status, account);
          old_message = 
            ap_savedstatus_get_message (current_ap_status, account);

          if ((old_type != new_type) || 
              ((old_message == NULL || new_substatus_message == NULL) &&
               (old_message != new_substatus_message)) ||
              (old_message != NULL && new_substatus_message != NULL &&
               strcmp (old_message, new_substatus_message))) 
          {
            updated = TRUE;
          } 
        }
      }
    }
  } else {
    substatus_messages = NULL;
  }

  /* And then the generic main message */
  sample_message = purple_savedstatus_get_message (template_status);
  if (sample_message) {
    new_message = ap_generate (sample_message, get_max_size_status (NULL, 
                                purple_savedstatus_get_type (template_status)));
  } else {
    new_message = NULL;
  }
  
  new_type = purple_savedstatus_get_type (template_status);

  if (!updated) {
    old_type = purple_savedstatus_get_type (current_ap_status);
    old_message = purple_savedstatus_get_message (current_ap_status);

    if ((old_type != new_type) || 
        ((old_message == NULL || new_message == NULL) &&
         (old_message != new_message)) ||
        (old_message != NULL && new_message != NULL &&
         strcmp (old_message, new_message))) 
    {
      updated = TRUE;
    } 
  }

  if (updated) {
    APMessageType type;
    PurpleSavedStatus *new_status;

    new_status = purple_savedstatus_new (NULL, new_type);

    if (new_message) {
      purple_savedstatus_set_message (new_status, new_message);
    }

    for (accounts = purple_accounts_get_all ();
         accounts != NULL;
         accounts = accounts->next) {
      account = (PurpleAccount *) accounts->data;
      substatus = purple_savedstatus_get_substatus (template_status, account);

      if (substatus != NULL) {
        substatus_type = purple_savedstatus_substatus_get_type (substatus);
        new_substatus_message = (gchar *) 
          g_hash_table_lookup (substatus_messages, account);
        purple_savedstatus_set_substatus (
          new_status, account, substatus_type, new_substatus_message);
        free (new_substatus_message);
      }        
       
      purple_savedstatus_activate_for_account (new_status, account); 
    }

    current_ap_status = new_status;

    if (new_type == PURPLE_STATUS_AVAILABLE) type = AP_MESSAGE_TYPE_AVAILABLE;
    else if (new_type == PURPLE_STATUS_AWAY) type = AP_MESSAGE_TYPE_AWAY;
    else                                   type = AP_MESSAGE_TYPE_STATUS;

    ap_gtk_add_message (AP_UPDATE_STATUS, type, new_message);
  }

  if (new_message) free (new_message);
  if (substatus_messages) {
    g_hash_table_destroy (substatus_messages); 
  }

  ap_update_queueing ();

  return updated;
}

static gboolean ap_update_cb (gpointer data) {
  gboolean result;
  guint timeout;
  guint delay;
 
  g_static_mutex_lock (&update_timeout_mutex);

  /* Start by removing timeout to self no matter what */
  timeout = GPOINTER_TO_INT (g_hash_table_lookup (update_timeouts, data));
  if (timeout) purple_timeout_remove (timeout);

  /* In future, check here if widget content has changed? */

  switch (GPOINTER_TO_INT (data)) {
    case AP_UPDATE_STATUS:
      result = ap_update_status ();
      break;
    case AP_UPDATE_PROFILE:
      result = ap_update_profile ();
      break;
    default:
      result = TRUE;
  }

  if (!result) {
    ap_debug ("general", "Content hasn't changed, updating later");
    delay = AP_SCHEDULE_UPDATE_DELAY;
  } else {
    ap_debug ("general", "Content updated");
    delay = 
      purple_prefs_get_int ("/plugins/gtk/autoprofile/delay_update") * 1000;
  }
  timeout = purple_timeout_add (delay, ap_update_cb, data);
  g_hash_table_insert (update_timeouts, data, GINT_TO_POINTER (timeout));

  g_static_mutex_unlock (&update_timeout_mutex);

  return FALSE;
}

void ap_update (APUpdateType type) 
{
  ap_update_cb (GINT_TO_POINTER (type));
}

void ap_update_after_delay (APUpdateType type)
{
  guint timeout;

  g_static_mutex_lock (&update_timeout_mutex);

  timeout = GPOINTER_TO_INT (g_hash_table_lookup (update_timeouts, 
    GINT_TO_POINTER (type)));
  if (timeout) purple_timeout_remove (timeout);

  timeout = purple_timeout_add (AP_SCHEDULE_UPDATE_DELAY, ap_update_cb, 
    GINT_TO_POINTER (type));
  g_hash_table_insert (update_timeouts, GINT_TO_POINTER (type), 
    GINT_TO_POINTER (timeout));

  g_static_mutex_unlock (&update_timeout_mutex);
}

void ap_update_stop (APUpdateType type)
{
  guint timeout;

  g_static_mutex_lock (&update_timeout_mutex);

  timeout = GPOINTER_TO_INT (g_hash_table_lookup (update_timeouts, 
    GINT_TO_POINTER (type)));
  if (timeout) purple_timeout_remove (timeout);

  g_hash_table_insert (update_timeouts, GINT_TO_POINTER (type), 0);

  g_static_mutex_unlock (&update_timeout_mutex);
}

static void ap_account_connected (PurpleConnection *gc) {
  ap_debug ("general", "Account connection detected");
  ap_update_after_delay (AP_UPDATE_PROFILE);
  ap_update_after_delay (AP_UPDATE_STATUS);
}

void ap_update_queueing () {
  if (ap_is_currently_away ()) {
    if (purple_prefs_get_bool(
      "/plugins/gtk/autoprofile/queue_messages_when_away")) {
      purple_prefs_set_string (PIDGIN_PREFS_ROOT "/conversations/im/hide_new", "away");
    } else {
      purple_prefs_set_string (PIDGIN_PREFS_ROOT "/conversations/im/hide_new", "never");
    }
  }
}

/* Called whenever current status is changed by Purple's status menu
   (in buddy list) */
static void ap_status_changed (
  const char *name, PurplePrefType type, gconstpointer val, gpointer data) {
  ap_debug ("general", "Status change detected");

  using_idleaway = FALSE;
  ap_autoaway_touch ();
  ap_update (AP_UPDATE_STATUS);
}

void ap_autoaway_enable () {
  ap_debug ("idle", "Using idleaway");

  using_idleaway = TRUE;
  ap_update (AP_UPDATE_STATUS);
}

void ap_autoaway_disable () {
  ap_debug ("idle", "Disabling idleaway");

  using_idleaway = FALSE;
  ap_update (AP_UPDATE_STATUS);
}

gboolean ap_autoaway_in_use () {
  return using_idleaway;
}

static gboolean ap_update_queue (gpointer data)
{
  PurpleAccount *account = NULL;
  PurpleConnection *gc = NULL;

  g_static_mutex_lock (&update_queue_mutex);

  if (queued_profiles != NULL) {
    account = (PurpleAccount *) queued_profiles->data;
    queued_profiles = queued_profiles->next;
  }

  g_static_mutex_unlock (&update_queue_mutex);

  gc = purple_account_get_connection (account);
  if (gc != NULL) {
    serv_set_info (gc, purple_account_get_user_info (account));
  }

  return TRUE;
}

static void ap_update_queue_start ()
{
  update_queue_timeout = purple_timeout_add (2000, ap_update_queue, NULL);
}

static void ap_update_queue_finish ()
{
  purple_timeout_remove (update_queue_timeout);
  update_queue_timeout = 0;
}
/*--------------------------------------------------------------------------* 
 * Preferences                                                              *
 *--------------------------------------------------------------------------*/
static void ap_delete_legacy_prefs () {
  if (purple_prefs_exists ("/plugins/gtk/autoprofile/tab_number")) {
    ap_debug ("general", "Deleting legacy preferences");

    purple_prefs_remove ("/plugins/gtk/autoprofile/components");

    purple_prefs_remove ("/plugins/gtk/autoprofile/tab_number");

    purple_prefs_remove ("/plugins/gtk/autoprofile/accounts/enable_away");
    purple_prefs_remove ("/plugins/gtk/autoprofile/accounts/enable_profile");
    purple_prefs_remove ("/plugins/gtk/autoprofile/accounts");

    purple_prefs_remove ("/plugins/gtk/autoprofile/message_titles");
    purple_prefs_remove ("/plugins/gtk/autoprofile/message_texts");

    purple_prefs_remove ("/plugins/gtk/autoprofile/default_profile");
    purple_prefs_remove ("/plugins/gtk/autoprofile/default_away");
    purple_prefs_remove ("/plugins/gtk/autoprofile/current_away");
    purple_prefs_remove ("/plugins/gtk/autoprofile/added_text");

    purple_prefs_remove ("/plugins/gtk/autoprofile/delay_profile");
    purple_prefs_remove ("/plugins/gtk/autoprofile/delay_away");

    purple_prefs_rename ("/plugins/gtk/autoprofile/text_respond",
                       "/plugins/gtk/autoprofile/autorespond/text");
    purple_prefs_rename ("/plugins/gtk/autoprofile/text_trigger",
                       "/plugins/gtk/autoprofile/autorespond/trigger");
    purple_prefs_rename ("/plugins/gtk/autoprofile/delay_respond",
                       "/plugins/gtk/autoprofile/autorespond/delay");
    purple_prefs_rename ("/plugins/gtk/autoprofile/use_trigger",
                       "/plugins/gtk/autoprofile/autorespond/enable");
  }
}

static void ap_init_preferences () {
  ap_debug ("general", "Initializing preference defaults if necessary");

  /* Adding the folders */
  purple_prefs_add_none ("/plugins/gtk");
  purple_prefs_add_none ("/plugins/gtk/autoprofile");
  purple_prefs_add_none ("/plugins/gtk/autoprofile/widgets");
  purple_prefs_add_none ("/plugins/gtk/autoprofile/autorespond");

  /* Behavior-settings */
  purple_prefs_add_int    ("/plugins/gtk/autoprofile/delay_update", 30);
  purple_prefs_add_string ("/plugins/gtk/autoprofile/show_summary", "always");
  purple_prefs_add_bool   ("/plugins/gtk/autoprofile/queue_messages_when_away",
    FALSE);
  purple_prefs_add_bool   ("/plugins/gtk/autoprofile/away_when_idle",
    purple_prefs_get_bool ("/purple/away/away_when_idle"));

  /* Auto-response settings */
  purple_prefs_add_string ("/plugins/gtk/autoprofile/autorespond/auto_reply",
    purple_prefs_get_string ("/purple/away/auto_reply"));
  purple_prefs_add_string ("/plugins/gtk/autoprofile/autorespond/text",
    _("Say the magic word if you want me to talk more!"));
  purple_prefs_add_string ("/plugins/gtk/autoprofile/autorespond/trigger", 
    _("please"));
  purple_prefs_add_int  ("/plugins/gtk/autoprofile/autorespond/delay", 2);
  purple_prefs_add_bool ("/plugins/gtk/autoprofile/autorespond/enable", TRUE);

  /* Profile settings */
  purple_prefs_add_string_list(
    "/plugins/gtk/autoprofile/profile_accounts", NULL);
  purple_prefs_add_string ("/plugins/gtk/autoprofile/profile",
    _("Get AutoProfile for Purple at <a href=\""
    "http://autoprofile.sourceforge.net/\">"
    "autoprofile.sourceforge.net</a><br><br>[Timestamp]"));
}

/*--------------------------------------------------------------------------* 
 * Last Call                                                                *
 *--------------------------------------------------------------------------*/
static void init_plugin (PurplePlugin *plugin) 
{ 
  ap_debug ("general", "Initializing AutoProfile");

  ap_init_preferences ();
  ap_widget_init ();
}

PURPLE_INIT_PLUGIN (autoprofile, init_plugin, info)


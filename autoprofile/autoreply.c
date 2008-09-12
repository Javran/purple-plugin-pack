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
#include "conversation.h"

#define SECS_BEFORE_RESENDING_AUTORESPONSE 600
#define SEX_BEFORE_RESENDING_AUTORESPONSE "Only after you're married"
#define MILLISECS_BEFORE_PROCESSING_MSG 100

static guint pref_cb;

static GSList *last_auto_responses = NULL;
struct last_auto_response {
  PurpleConnection *gc;
  char name[80];
  time_t sent;
};

static time_t response_timeout = 0;

/*--------------------------------------------------------------------------*
 * Auto-response utility functions                                          *
 *--------------------------------------------------------------------------*/
static gboolean
expire_last_auto_responses(gpointer data)
{
  GSList *tmp, *cur;
  struct last_auto_response *lar;

  tmp = last_auto_responses;

  while (tmp) {
    cur = tmp;
    tmp = tmp->next;
    lar = (struct last_auto_response *)cur->data;

    if ((time(NULL) - lar->sent) > SECS_BEFORE_RESENDING_AUTORESPONSE) {
      last_auto_responses = g_slist_remove(last_auto_responses, lar);
      g_free(lar);
    }
  }

  return FALSE; /* do not run again */
}

static struct last_auto_response *
get_last_auto_response(PurpleConnection *gc, const char *name)
{
  GSList *tmp;
  struct last_auto_response *lar;

  /* because we're modifying or creating a lar, schedule the
   * function to expire them as the pref dictates */
  purple_timeout_add((SECS_BEFORE_RESENDING_AUTORESPONSE + 5) * 1000, 
    expire_last_auto_responses, NULL);

  tmp = last_auto_responses;

  while (tmp) {
    lar = (struct last_auto_response *)tmp->data;

    if (gc == lar->gc && !strncmp(name, lar->name, sizeof(lar->name)))
      return lar;

    tmp = tmp->next;
  }

  lar = (struct last_auto_response *)g_new0(struct last_auto_response, 1);
  g_snprintf(lar->name, sizeof(lar->name), "%s", name);
  lar->gc = gc;
  lar->sent = 0;
  last_auto_responses = g_slist_append(last_auto_responses, lar);

  return lar;
}

/*--------------------------------------------------------------------------*
 * Message send/receive general functionality                               *
 *--------------------------------------------------------------------------*/
/* Detecting sent message stuff */
static void sent_im_msg_cb (PurpleAccount *account, const char *receiver,
                     const char *message) 
{
  PurpleConnection *gc;
  PurplePresence *presence;
  const gchar *auto_reply_pref;

  gc = purple_account_get_connection (account);
  presence = purple_account_get_presence (account);

  /*
   * FIXME - If "only auto-reply when away & idle" is set, then shouldn't
   * this only reset lar->sent if we're away AND idle?
   */
  auto_reply_pref = 
    purple_prefs_get_string ("/plugins/gtk/autoprofile/autorespond/auto_reply");
  if ((gc->flags & PURPLE_CONNECTION_AUTO_RESP) &&
      !purple_presence_is_available(presence) &&
      strcmp(auto_reply_pref, "never")) 
  {
    struct last_auto_response *lar;
    lar = get_last_auto_response(gc, receiver);
    lar->sent = time(NULL);
  }
}

/* Detecting received message stuff */
struct received_im_msg {
  PurpleAccount *account;
  char *sender;
  char *message;
};

static gint process_received_im_msg (gpointer data) 
{
  struct received_im_msg *received_im;
  PurpleAccount *account;
  char *sender;
  char *message;
  PurpleConnection *gc;
  PurpleConversation *conv;

  received_im = (struct received_im_msg *) data;
  account = received_im->account;
  sender = received_im->sender;
  message = received_im->message;
  free (data);

  gc = purple_account_get_connection (account);

  /* search for conversation again in case it was created by other handlers */
  conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, 
    sender, gc->account);
  if (conv == NULL)
    conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, account, sender);

  /*
   * Don't autorespond if:
   *
   *  - it's not supported on this connection
   *  - we are available
   *  - or it's disabled
   *  - or we're not idle and the 'only auto respond if idle' pref
   *    is set
   */ 
  if (gc->flags & PURPLE_CONNECTION_AUTO_RESP)
  {
    PurplePresence *presence;
    PurpleStatus *status;
    PurpleStatusType *status_type;
    PurpleStatusPrimitive primitive;
    const gchar *auto_reply_pref;
    char *away_msg = NULL;

    auto_reply_pref = purple_prefs_get_string(
      "/plugins/gtk/autoprofile/autorespond/auto_reply");

    presence = purple_account_get_presence(account);
    status = purple_presence_get_active_status(presence);
    status_type = purple_status_get_type(status);
    primitive = purple_status_type_get_primitive(status_type);
    if ((primitive == PURPLE_STATUS_AVAILABLE) ||
        (primitive == PURPLE_STATUS_INVISIBLE) ||
        (primitive == PURPLE_STATUS_MOBILE) ||
        !strcmp(auto_reply_pref, "never") ||
        (!purple_presence_is_idle(presence) && 
         !strcmp(auto_reply_pref, "awayidle")))
    {
      free (sender);
      free (message);
      return FALSE;
    }

    away_msg = ap_get_sample_status_message (account);

    if ((away_msg != NULL) && (*away_msg != '\0')) {
      struct last_auto_response *lar;
      gboolean autorespond_enable;
      time_t now = time(NULL);

      autorespond_enable = purple_prefs_get_bool (
        "/plugins/gtk/autoprofile/autorespond/enable");
      /*
       * This used to be based on the conversation window. But um, if
       * you went away, and someone sent you a message and got your
       * auto-response, and then you closed the window, and then they
       * sent you another one, they'd get the auto-response back too
       * soon. Besides that, we need to keep track of this even if we've
       * got a queue. So the rest of this block is just the auto-response,
       * if necessary.
       */
      lar = get_last_auto_response(gc, sender);
      if ((now - lar->sent) >= SECS_BEFORE_RESENDING_AUTORESPONSE) {
        lar->sent = now;
        // Send basic autoresponse
        serv_send_im (gc, sender, away_msg, PURPLE_MESSAGE_AUTO_RESP);
        purple_conv_im_write (PURPLE_CONV_IM(conv), NULL, away_msg,
                            PURPLE_MESSAGE_SEND | PURPLE_MESSAGE_AUTO_RESP, 
                            now);

        // Send additional hint if enabled
        if (autorespond_enable) {
          const gchar *query = purple_prefs_get_string (
            "/plugins/gtk/autoprofile/autorespond/text");
          serv_send_im (gc, sender, query, PURPLE_MESSAGE_AUTO_RESP);
          purple_conv_im_write (PURPLE_CONV_IM (conv), NULL, query,
                              PURPLE_MESSAGE_SEND | PURPLE_MESSAGE_AUTO_RESP,
                              now); 
        }

      } else if (autorespond_enable && 
          difftime (time(NULL), response_timeout) >
          purple_prefs_get_int ("/plugins/gtk/autoprofile/autorespond/delay")) {
        gchar *text = purple_markup_strip_html (message);
        if (match_start (text, purple_prefs_get_string (
                      "/plugins/gtk/autoprofile/autorespond/trigger")) == 1) {
          serv_send_im (gc, sender, away_msg, PURPLE_MESSAGE_AUTO_RESP);
          purple_conv_im_write (PURPLE_CONV_IM (conv), NULL, away_msg,
                              PURPLE_MESSAGE_SEND | PURPLE_MESSAGE_AUTO_RESP,
                              now);       

          response_timeout = time (NULL);
          ap_debug ("autorespond", "string matched, responding");
        }
        free (text);
      }
    }

    free (away_msg);
  }

  free (sender);
  free (message);

  return FALSE;
}

static void received_im_msg_cb (PurpleAccount *account, char *sender, 
        char *message, PurpleConversation *conv, PurpleMessageFlags flags)
{
  struct received_im_msg *received_im;

  received_im = 
    (struct received_im_msg *) malloc (sizeof (struct received_im_msg));
  received_im->account = account;
  received_im->sender = strdup (sender);
  received_im->message = strdup (message);

  purple_timeout_add (MILLISECS_BEFORE_PROCESSING_MSG, process_received_im_msg,
    received_im);
}

static void auto_pref_cb (
  const char *name, PurplePrefType type, gconstpointer val, gpointer data) 
{
  if (!strcmp (purple_prefs_get_string ("/purple/away/auto_reply"), "never"))
    return;

  purple_notify_error (NULL, NULL,
    N_("This preference is disabled"), 
    N_("This preference currently has no effect because AutoProfile is in "
       "use.  To modify this behavior, use the AutoProfile configuration "
       "menu."));

  purple_prefs_set_string ("/purple/away/auto_reply", "never");
}

/*--------------------------------------------------------------------------*
 * Global functions                                                         *
 *--------------------------------------------------------------------------*/
void ap_autoreply_start ()
{
  purple_prefs_set_string ("/purple/away/auto_reply", "never");

  purple_signal_connect (purple_conversations_get_handle (), "sent-im-msg",
    ap_get_plugin_handle (), PURPLE_CALLBACK(sent_im_msg_cb), NULL);
  purple_signal_connect (purple_conversations_get_handle (), "received-im-msg",
    ap_get_plugin_handle (), PURPLE_CALLBACK(received_im_msg_cb), NULL);

  pref_cb = purple_prefs_connect_callback (ap_get_plugin_handle (),
    "/purple/away/auto_reply", auto_pref_cb, NULL);
}

void ap_autoreply_finish ()
{
  GSList *tmp;

  // Assumes signals are disconnected globally
  
  purple_prefs_disconnect_callback (pref_cb);
  pref_cb = 0;

  purple_prefs_set_string ("/purple/away/auto_reply", purple_prefs_get_string (
    "/plugins/gtk/autoprofile/autorespond/auto_reply"));

  while (last_auto_responses) {
    tmp = last_auto_responses->next;
    g_free (last_auto_responses->data);
    g_slist_free_1 (last_auto_responses);
    last_auto_responses = tmp;
  }
}


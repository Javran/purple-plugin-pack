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
#include "log.h"
#include "account.h"
#include "conversation.h"
#include "utility.h"
#include "util.h"

#include "comp_logstats.h"

struct conversation_time {
  time_t *start_time;
  char *name;
};

/* Represents data about a particular 24 hour period in the logs */
struct log_date {
  int year;                   // The year
  int month;                  // The month
  int day;                    // The day
  int received_msgs;          // # msgs received
  int received_words;         // # words received
  int sent_msgs;              // # msgs sent
  int sent_words;             // # words sent
  GSList *conversation_times; // List of conversation_time pointers
};

/* List of struct log_dates 
   This is SORTED by most recent first */
static GSList *dates = NULL;

/* Hashtable of log_dates */
static GHashTable *dates_table = NULL;

/* Is the current line part of a message sent or received? */
static gboolean receiving = FALSE;
/* Shortcut vars */
static char *cur_receiver = NULL;
static char *cur_sender = NULL;

/* Implements GCompareFunc */
static gint conversation_time_compare (gconstpointer x, gconstpointer y) {
  const struct conversation_time *a = x;
  const struct conversation_time *b = y;

  if (difftime (*(a->start_time), *(b->start_time)) == 0.0) {
    if (!strcmp (a->name, b->name))
      return 0;
  }

  return -1;
}

/* Implements GCompareFunc */
static gint log_date_compare (gconstpointer x, gconstpointer y)
{
  const struct log_date *a = y;
  const struct log_date *b = x;

  if (a->year == b->year) {
    if (a->month == b->month) {
      if (a->day == b->day)
        return 0;
      else
        return a->day - b->day;
    } else {
      return a->month - b->month;
    }
  } else {
    return a->year - b->year;
  }
}

/* Implements GHashFunc */
static guint log_date_hash (gconstpointer key)
{
  const struct log_date *d = key;
  return ((d->year * 365) + (d->month * 12) + (d->day));
}

/* Implements GEqualFunc */
static gboolean log_date_equal (gconstpointer x, gconstpointer y)
{
  const struct log_date *a = y;
  const struct log_date *b = x;

  if (a->year == b->year &&
      a->month == b->month &&
      a->day == b->day) {
    return TRUE;
  }
  return FALSE;
}

/* Returns the struct log_date associated with a particular date.
   Will MODIFY list of dates and insert sorted if not yet created */
static struct log_date *get_date (int year, int month, int day)
{
  struct log_date *cur_date;
  gpointer *node;

  cur_date = (struct log_date *)malloc(sizeof(struct log_date));
  cur_date->year = year;
  cur_date->month = month;
  cur_date->day = day;

  if ((node = g_hash_table_lookup (dates_table, cur_date))) {
    free (cur_date);
    return (struct log_date *)node;
  } else {
    g_hash_table_insert (dates_table, cur_date, cur_date);
    cur_date->received_msgs = 0;
    cur_date->received_words = 0;
    cur_date->sent_msgs = 0;
    cur_date->sent_words = 0;
    cur_date->conversation_times = NULL;
    return cur_date;
  }
}

/* Like get_date, except specific to the current date */
static struct log_date *get_today ()
{
  time_t the_time;
  struct tm *cur_time;

  time (&the_time);
  cur_time = localtime (&the_time);

  return get_date (cur_time->tm_year, cur_time->tm_mon, cur_time->tm_mday);
}

static int string_word_count (const char *line)
{
  int count, state;

  count = 0;
  state = 0;

  /* If state is 1, currently processing a word */
  while (*line) {
    if (state == 0) {
      if (!isspace (*line))
        state = 1;
    } else {
      if (isspace (*line)) {
        state = 0;
        count++;
      }
    }
    line++;
  }

  if (state == 1)
    count++;

  return count;
}

/* Figure out if a person is yourself or someone else */
static gboolean is_self (PurpleAccount *a, const char *name) {
  GList *accounts, *aliases, *aliases_start;
  PurpleAccount *account;

  char *normalized;
  const char *normalized_alias;

  if (cur_sender && !strcmp (cur_sender, name)) {
    return TRUE;
  }

  if (cur_receiver && !strcmp (cur_receiver, name)) {
    return FALSE;
  }

  normalized = strdup (purple_normalize (a, name));
  accounts = purple_accounts_get_all ();

  aliases_start = aliases = purple_prefs_get_string_list (
    "/plugins/gtk/autoprofile/components/logstat/aliases");

  while (aliases) {
    normalized_alias = purple_normalize (a, (char *)aliases->data);

    if (!strcmp (normalized, normalized_alias)) {
      free_string_list (aliases_start);
      free (normalized);

      if (cur_sender)
        free (cur_sender);
      cur_sender = strdup (name);

      return TRUE;
    }

    aliases = aliases->next;
  }

  free_string_list (aliases_start);

  while (accounts) {
    account = (PurpleAccount *)accounts->data;
    if (!strcmp (normalized, purple_account_get_username (account))) {
      free (normalized);
      if (cur_sender)
        free (cur_sender);
      cur_sender = strdup (name);
      return TRUE;
    }
    accounts = accounts->next;
  }

  free (normalized);

  if (cur_receiver)
    free (cur_receiver);
  cur_receiver = strdup (name);
  return FALSE;
}

/* Parses a line of a conversation */
static void parse_line (PurpleLog *cur_log, char *l, struct log_date *d)
{
  char *cur_line, *cur_line_start;
  char *name;
  char *message;

  char *line = l;

  if (strlen (line) > 14 && *line == ' ')
    line++;

  if (strlen (line) > 13 &&
      *line == '(' &&
      isdigit (*(line + 1)) &&
      isdigit (*(line + 2)) &&
      *(line + 3) == ':' &&
      isdigit (*(line + 4)) &&
      isdigit (*(line + 5)) &&
      *(line + 6) == ':' &&
      isdigit (*(line + 7)) &&
      isdigit (*(line + 8)) &&
      *(line + 9) == ')' &&
      isspace (*(line + 10))) {
    cur_line_start = cur_line = line + 11;
    while (*cur_line) {
      if (*cur_line == ':') {
        *cur_line = '\0';
        name = cur_line_start;
        message = ++cur_line;

        receiving = !is_self (cur_log->account, name);

        if (receiving) {
          d->received_msgs++;
          d->received_words += string_word_count (message);
        } else {
          d->sent_msgs++;
          d->sent_words += string_word_count (message);
        }

        return;
      }
      cur_line++;
    }

  }

  if (receiving) {
    d->received_words += string_word_count (line);
  } else {
    d->sent_words += string_word_count (line);
  }
}

/* Parses a conversation if hasn't been handled yet */
static void parse_log (PurpleLog *cur_log)
{
  struct log_date *the_date;
  struct tm *the_time;
  struct conversation_time *conv_time;

  PurpleLogReadFlags flags;
  char *content, *cur_content, *cur_content_start, *temp;

  the_time = localtime (&(cur_log->time));
  the_date = get_date (the_time->tm_year, the_time->tm_mon, the_time->tm_mday);

  /* Check for old log and if no conflicts, add to list */
  conv_time = (struct conversation_time *)malloc (
    sizeof (struct conversation_time));
  conv_time->start_time = (time_t *)malloc (sizeof(time_t));
  *(conv_time->start_time) = cur_log->time;
  conv_time->name = strdup (cur_log->name);

  if (g_slist_find_custom (the_date->conversation_times, conv_time, 
    conversation_time_compare)) {
    /* We already processed this!  Halt! */
    free (conv_time->start_time);
    free (conv_time->name);
    free (conv_time);
    return;
  }

  the_date->conversation_times = g_slist_prepend (the_date->conversation_times,
    conv_time);

  /* Start rolling the counters! */
  temp = purple_log_read (cur_log, &flags);
  if (!strcmp ("html", cur_log->logger->id)) {
    content = purple_markup_strip_html (temp);
    free (temp);
  } else {
    content = temp;
  }

  cur_content_start = cur_content = content;

  /* Splits the conversation into lines (each line may not necessarily
     be a seperate message */
  while (*cur_content) {
    if (*cur_content == '\n') {
      *cur_content = '\0';
      parse_line (cur_log, cur_content_start, the_date);
      cur_content_start = cur_content + 1;
    }
    cur_content++;
  }

  parse_line (cur_log, cur_content_start, the_date);

  free (content);
}

/* Get names of users in logs */
static GList *logstats_get_names (PurpleLogType type, PurpleAccount *account)
{
  GDir *dir;
  const char *prpl;
  GList *ret;
  const char *filename;
  char *path, *me, *tmp;

  ret = NULL;

  if (type == PURPLE_LOG_CHAT)
    me = g_strdup_printf ("%s.chat", purple_normalize(account, 
      purple_account_get_username(account)));
  else
    me = g_strdup (purple_normalize(account, 
      purple_account_get_username(account)));

  /* Get the old logger names */
  path = g_build_filename(purple_user_dir(), "logs", NULL);
  if (!(dir = g_dir_open(path, 0, NULL))) {
    g_free(path);
    return ret;
  }

  while ((filename = g_dir_read_name (dir))) {
    if (purple_str_has_suffix (filename, ".log")) {
      tmp = strdup (filename);
      *(tmp + strlen (filename) - 4) = '\0';
      if (!string_list_find (ret, tmp))
        ret = g_list_prepend (ret, strdup (tmp));
      free (tmp);
    }
  }

  g_dir_close (dir);
  g_free (path);

  /* Get the account-specific names */
  prpl = PURPLE_PLUGIN_PROTOCOL_INFO
    (purple_find_prpl (purple_account_get_protocol_id(account)))->list_icon(
      account, NULL);

  path = g_build_filename(purple_user_dir(), "logs", prpl, me, NULL);
  g_free (me);

  if (!(dir = g_dir_open(path, 0, NULL))) {
    g_free(path);
    return ret;
  }

  while ((filename = g_dir_read_name (dir))) {
    if (!string_list_find (ret, filename))
      ret = g_list_prepend (ret, strdup (filename));
  }

  g_dir_close (dir);
  g_free (path);

  return ret;
}

/* On load, reads in all logs and initializes stats database */
static void logstats_read_logs ()
{
  GList *accounts, *logs, *logs_start, *names, *names_start;
  PurpleLog *cur_log;

  accounts = purple_accounts_get_all();

  ap_debug ("logstats", "parsing log files");

  while (accounts) {
    names_start = names = logstats_get_names (PURPLE_LOG_IM, 
      (PurpleAccount *)accounts->data);

    while (names) {
      logs_start = purple_log_get_logs (PURPLE_LOG_IM, (char *)names->data,
        (PurpleAccount *)accounts->data);
      logs = logs_start;

      while (logs) {
        cur_log = (PurpleLog *)logs->data;
        parse_log (cur_log);
        purple_log_free (cur_log);
        logs = logs->next;
      }

      g_list_free (logs_start);
      names = names->next;
    }

    free_string_list (names_start);
    accounts = accounts->next;
  }
 
  /* Cleanup */

  ap_debug ("logstats", "finished parsing log files");
}

/* Implements GHFunc */
static void add_element (gpointer key, gpointer value, gpointer data)
{
  dates = g_slist_insert_sorted (dates, value, log_date_compare);
}

/* Updates GList against hashtable */
static void logstats_update_dates ()
{
  g_slist_free (dates);
  dates = NULL;
  g_hash_table_foreach (dates_table, add_element, NULL);
}

/*--------------------- Total calculations -------------------*/
static int get_total (const char *field)
{
  GSList *cur_day;
  int count;
  struct log_date *d;

  cur_day = dates;
  count = 0;
  while (cur_day) {
    d = (struct log_date *)cur_day->data;
    if (!strcmp (field, "received_msgs")) {
      count += d->received_msgs;
    } else if (!strcmp (field, "received_words")) {
      count += d->received_words;
    } else if (!strcmp (field, "sent_msgs")) {
      count += d->sent_msgs;
    } else if (!strcmp (field, "sent_words")) {
      count += d->sent_words;
    } else if (!strcmp (field, "num_convos")) {
      count += g_slist_length (d->conversation_times);
    } 

    cur_day = cur_day->next;
  }

  return count;
}

static int get_recent_total (const char *field, int hours)
{
  GSList *cur_day;
  int count;
  struct log_date *d;
  time_t cur_day_time;

  cur_day = dates;
  count = 0;

  while (cur_day) {
    d = (struct log_date *)cur_day->data;
    cur_day_time = purple_time_build (d->year + 1900, d->month + 1, d->day, 
      0, 0, 0);
    if (difftime (time (NULL), cur_day_time) > (double) hours * 60.0 * 60.0)
      break;

    if (!strcmp (field, "received_msgs")) {
      count += d->received_msgs;
    } else if (!strcmp (field, "sent_msgs")) {
      count += d->sent_msgs;
    } else if (!strcmp (field, "num_convos")) {
      count += g_slist_length (d->conversation_times);
    } 

    cur_day = cur_day->next;
  }

  return count;
}

static int num_days_since_start ()
{
  GSList *first_day;
  double difference;
  struct log_date *d;

  first_day = g_slist_last (dates);

  if (!first_day)
    return 0;

  d = (struct log_date *)first_day->data;

  difference = difftime (
    time (NULL), purple_time_build (d->year + 1900, d->month + 1, d->day, 
      0, 0, 0));

  return (int) difference / (60.0 * 60.0 * 24.0); 
}

static struct log_date *get_max_date (const char *field)
{
  struct log_date *max_date, *cur_date;
  int max_so_far, cur_max;
  GSList *cur_day;

  max_so_far = 0;
  max_date = NULL;
  cur_day = dates;

  while (cur_day) {
    cur_date = (struct log_date *)cur_day->data;
    if (!strcmp (field, "conversations")) {
      cur_max = g_slist_length (cur_date->conversation_times);
    } else if (!strcmp (field, "received")) {
      cur_max = cur_date->received_msgs;
    } else if (!strcmp (field, "sent")) {
      cur_max = cur_date->sent_msgs;
    } else if (!strcmp (field, "total")) {
      cur_max = cur_date->sent_msgs + cur_date->received_msgs;
    } else {
      cur_max = 0;
    }

    if (cur_max >= max_so_far) {
      max_date = cur_date;
      max_so_far = cur_max;
    }

    cur_day = cur_day->next;
  }

  return max_date;
}

static char *date_string (const char *field)
{
  struct log_date *d;
  char *output;
  struct tm *t_struct;
  time_t t;
  GSList *last_day;

  last_day = g_slist_last (dates);

  if (!last_day)
    return NULL;

  if (!strcmp (field, "first")) {
    d = (struct log_date *) last_day->data;
  } else {
    d = get_max_date (field);
  }

  if (!d)
    return NULL;

  output = (char *)malloc (sizeof(char) * AP_SIZE_MAXIMUM);
  t_struct = (struct tm *)malloc(sizeof(struct tm));
  t_struct->tm_year = d->year;
  t_struct->tm_mon = d->month;
  t_struct->tm_mday = d->day;
  t_struct->tm_sec = 0;
  t_struct->tm_min = 0;
  t_struct->tm_hour = 0;
  t = mktime (t_struct);
  free (t_struct);
  t_struct = localtime (&t);

  strftime (output, AP_SIZE_MAXIMUM - 1, "%a %b %d, %Y", t_struct);
  return output; 
}

static int get_max (const char *field)
{
  struct log_date *max_date = get_max_date (field);

  if (!max_date)
    return 0;
  
  if (!strcmp (field, "conversations")) {
    return g_slist_length (max_date->conversation_times);
  } else if (!strcmp (field, "received")) {
    return max_date->received_msgs;
  } else if (!strcmp (field, "sent")) {
    return max_date->sent_msgs;
  } else if (!strcmp (field, "total")) {
    return max_date->sent_msgs + max_date->received_msgs;
  } else {
    ap_debug ("logstats", "get-max: invalid paramater");
    return 0;
  }

}


/*--------------------- Signal handlers ----------------------*/
static void logstats_received_im (PurpleAccount *account, char *sender,
                                  char *message, int flags)
{
  struct log_date *the_date; 

  the_date = get_today ();
  the_date->received_msgs++;
  the_date->received_words += string_word_count (message);

  receiving = TRUE;
}

static void logstats_sent_im (PurpleAccount *account, const char *receiver,
                              const char *message)
{
  struct log_date *the_date;

  the_date = get_today ();  
  the_date->sent_msgs++;
  the_date->sent_words += string_word_count (message);

  receiving = FALSE;
}

static void logstats_conv_created (PurpleConversation *conv)
{
  struct log_date *the_date;
  struct conversation_time *the_time;
  
  if (conv->type == PURPLE_CONV_TYPE_IM) {
    the_time = malloc (sizeof(struct conversation_time));
    the_time->name = strdup (conv->name);
    the_time->start_time = malloc (sizeof(time_t));
    time (the_time->start_time);

    the_date = get_today ();
    the_date->conversation_times = g_slist_prepend (
      the_date->conversation_times, the_time);

    logstats_update_dates ();
  }
}

/*--------------------------- Main functions -------------------------*/

/* Component load */
void logstats_load ()
{
  int count;
  char *msg;

  if (!purple_prefs_get_bool (
    "/plugins/gtk/autoprofile/components/logstat/enabled")) {
    return;
  }

  /* Initialize database */
  dates_table = g_hash_table_new (log_date_hash, log_date_equal);
  logstats_read_logs ();
  logstats_update_dates ();

  /* Debug */
  msg = (char *)malloc (sizeof(char) * AP_SIZE_MAXIMUM);
  count = get_total ("received_msgs");
  g_snprintf (msg, AP_SIZE_MAXIMUM, "received msg total is %d", count);
  ap_debug ("logstats", msg);
  count = get_total ("sent_msgs");
  g_snprintf (msg, AP_SIZE_MAXIMUM, "sent msg total is %d", count);
  ap_debug ("logstats", msg);
  count = get_total ("received_words");
  g_snprintf (msg, AP_SIZE_MAXIMUM, "received word total is %d", count);
  ap_debug ("logstats", msg);
  count = get_total ("sent_words");
  g_snprintf (msg, AP_SIZE_MAXIMUM, "sent word total is %d", count);
  ap_debug ("logstats", msg);
  count = get_total ("num_convos");
  g_snprintf (msg, AP_SIZE_MAXIMUM, "num conversations is %d", count);
  ap_debug ("logstats", msg);
  count = g_slist_length (dates);
  g_snprintf (msg, AP_SIZE_MAXIMUM, "num days with conversations is %d", count);
  ap_debug ("logstats", msg);

  free(msg);

  /* Connect signals */
  purple_signal_connect (purple_conversations_get_handle (),
                       "received-im-msg", ap_get_plugin_handle (),
                       PURPLE_CALLBACK (logstats_received_im), NULL);
  purple_signal_connect (purple_conversations_get_handle (),
                       "sent-im-msg", ap_get_plugin_handle (),
                       PURPLE_CALLBACK (logstats_sent_im), NULL);
  purple_signal_connect (purple_conversations_get_handle (),
                       "conversation-created", ap_get_plugin_handle (),
                       PURPLE_CALLBACK (logstats_conv_created), NULL);
}

/* Component unload */
void logstats_unload ()
{
  struct log_date *cur_date;
  struct conversation_time *cur_time;
  GSList *temp;

  if (!purple_prefs_get_bool (
    "/plugins/gtk/autoprofile/components/logstat/enabled")) {
    return;
  }

  /* Disconnect signals */
  purple_signal_disconnect (purple_conversations_get_handle (),
                          "received-im-msg", ap_get_plugin_handle (),
                          PURPLE_CALLBACK (logstats_received_im));
  purple_signal_disconnect (purple_conversations_get_handle (),
                          "sent-im-msg", ap_get_plugin_handle (),
                          PURPLE_CALLBACK (logstats_sent_im));
  purple_signal_disconnect (purple_conversations_get_handle (),
                          "conversation-created", ap_get_plugin_handle (),
                          PURPLE_CALLBACK (logstats_conv_created));

  logstats_update_dates ();

  /* Free all the memory */
  while (dates) {
    cur_date = (struct log_date *)dates->data;
    while (cur_date->conversation_times) {
      temp = cur_date->conversation_times;
      cur_time = (struct conversation_time *)temp->data;
      cur_date->conversation_times = temp->next;
      free (cur_time->start_time);
      free (cur_time->name);
      free (cur_time);
      g_slist_free_1 (temp);
    }
    free (cur_date);
    temp = dates;
    dates = dates->next; 
    g_slist_free_1 (temp);
  }

  if (cur_receiver) {
    free (cur_receiver);
    cur_receiver = NULL;
  }
  if (cur_sender) {
    free (cur_sender);
    cur_sender = NULL;
  }
  g_hash_table_destroy (dates_table);
  dates_table = NULL;
}

/* Generate the output */
static char *logstats_generate ()
{ 
  char *buf, *output, *date;
  int state;
  const char *format;

  if (!purple_prefs_get_bool (
    "/plugins/gtk/autoprofile/components/logstat/enabled")) {
    return NULL;
  }

  format = purple_prefs_get_string (
    "/plugins/gtk/autoprofile/components/logstat/format");

  output = (char *)malloc (sizeof(char)*AP_SIZE_MAXIMUM);
  *output = '\0';
  buf = (char *)malloc (sizeof(char)*AP_SIZE_MAXIMUM);
  *buf = '\0';

  state = 0;

  while (*format) {
    if (state == 1) {
      switch (*format) {
        case '%':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%c", output, *format);
          break;
        case 'R':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output, get_total ("received_msgs"));
          break;
        case 'r':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output, get_total ("received_words"));
          break;
        case 'S':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output, get_total ("sent_msgs"));
          break;
        case 's':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output, get_total ("sent_words"));
          break;
        case 'T':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output, 
            get_total ("sent_msgs") + get_total ("received_msgs"));
          break;
        case 't':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output, 
            get_total ("sent_words") + get_total ("received_words"));
          break;
        case 'D':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output,
            num_days_since_start ());
          break;
        case 'd':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output, g_slist_length (dates));
          break;
        case 'N':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output, get_total ("num_convos"));
          break;
        case 'n':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%.2f", output,
            (double) get_total ("num_convos") / (double) g_slist_length (dates));
          break;
        case 'i':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output, get_max ("conversations"));
          break;
        case 'I':
          date = date_string ("conversations");
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%s", output, date);
          free (date);
          break;
        case 'j':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output, get_max ("sent"));
          break;
        case 'J':
          date = date_string ("sent");
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%s", output, date);
          free (date);
          break;
        case 'k':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output, get_max ("received"));
          break;
        case 'K':
          date = date_string ("received");
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%s", output, date);
          free (date);
          break;
        case 'l':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output, get_max ("total"));
          break;
        case 'L':
          date = date_string ("total");
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%s", output, date);
          free (date);
          break;
        case 'f':
          date = date_string ("first");
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%s", output, date);
          free (date);
          break;
        case 'u':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%.2f", output, 
            (double) get_total ("received_words") / (double) get_total ("received_msgs"));
          break;
        case 'v':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%.2f", output, 
            (double) get_total ("sent_words") / (double) get_total ("sent_msgs"));
          break;
        case 'w':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%.2f", output, 
            (double) (get_total ("received_words") + get_total ("sent_words")) / (double) (get_total("received_msgs") + get_total ("sent_msgs")));
          break;
        case 'U':
         g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%.2f", output, 
            (double) get_total ("received_msgs") / (double) get_total ("num_convos"));
          break;
        case 'V':
         g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%.2f", output, 
            (double) get_total ("sent_msgs") / (double) get_total ("num_convos"));
          break;
        case 'W':
         g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%.2f", output, 
            (double) (get_total ("received_msgs") + get_total ("sent_msgs")) / (double) get_total("num_convos"));
          break;
        case 'x':
         g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%.2f", output, 
            (double) get_total ("received_words") / (double) g_slist_length (dates));
          break;
        case 'y':
         g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%.2f", output, 
            (double) get_total ("sent_words") / (double) g_slist_length (dates));
          break;
        case 'z':
         g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%.2f", output, 
            ((double) get_total ("received_words") + (double) get_total ("sent_words")) / (double) g_slist_length (dates));
          break;
        case 'X':
         g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%.2f", output, 
            (double) get_total ("received_msgs") / (double) g_slist_length (dates));
          break;
        case 'Y':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%.2f", output, 
            (double) get_total ("sent_msgs") / (double) g_slist_length (dates));
          break;
        case 'Z':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%.2f", output, 
            (double) (get_total ("received_msgs") + get_total ("sent_msgs")) / (double) g_slist_length (dates));
          break;
        case 'p':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%.1f", output,
            100.0 * (double) g_slist_length (dates) / (double) num_days_since_start ());
          break;
        case 'a':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output,
            ((struct log_date *) dates->data)->received_msgs);
          break;
        case 'b':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output,
            ((struct log_date *) dates->data)->sent_msgs);
          break;
        case 'c':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output,
            g_slist_length (((struct log_date *) dates->data)->conversation_times));
          break;
        case 'e':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output,
            ((struct log_date *) dates->data)->sent_msgs + ((struct log_date *) dates->data)->received_msgs);
          break;
        case 'A':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output, get_recent_total ("received_msgs", 24 * 7));
          break;
        case 'B':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output, get_recent_total ("sent_msgs", 24 * 7));
          break;
        case 'C':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output, get_recent_total ("num_convos", 24 * 7));
          break;
        case 'E':
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%d", output, 
            get_recent_total ("received_msgs", 24 * 7) + get_recent_total ("received_msgs", 24 * 7));
          break;
        default:
          g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%c", output, *format);
          break;
      }

      strcpy (output, buf);
      format++;
      state = 0;
    } else {
      if (*format == '%') {
        state = 1;
      } else {
        g_snprintf (buf, AP_SIZE_MAXIMUM, "%s%c", output, *format);
        strcpy (output, buf);
      }
      format++;
    }

  }

  free (buf);
  return output;
}

/* Initialize preferences */
static void logstats_init ()
{
  purple_prefs_add_none ("/plugins/gtk/autoprofile/components/logstat");
  purple_prefs_add_bool (
    "/plugins/gtk/autoprofile/components/logstat/enabled", FALSE);
  purple_prefs_add_string (
    "/plugins/gtk/autoprofile/components/logstat/format", "");
  purple_prefs_add_string_list (
    "/plugins/gtk/autoprofile/components/logstat/aliases", NULL);
}

/* The heart of the component */
static char *identifiers [7] = { 
  N_("logs"), 
  N_("log"), 
  N_("stat"), 
  N_("stats"), 
  N_("logstats"),
  N_("log statistics"),
  NULL
};

struct component logstats =
{
  N_("Purple log statistics"),
  N_("Display various statistics about your message and system logs"),
  identifiers,
  &logstats_generate,
  &logstats_init,
  &logstats_load,
  &logstats_unload,
  NULL,
  &logstats_prefs
};


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

#include "debug.h"

/* TODO: get rid of this and port to glib time */
static GStaticMutex time_mutex = G_STATIC_MUTEX_INIT;

static struct tm *ap_tm_copy (const struct tm *t) {
  struct tm *r;
  r = (struct tm *) malloc (sizeof (struct tm));
  r->tm_sec   = t->tm_sec;
  r->tm_min   = t->tm_min;
  r->tm_hour  = t->tm_hour;
  r->tm_mday  = t->tm_mday;
  r->tm_mon   = t->tm_mon;
  r->tm_year  = t->tm_year;
  r->tm_wday  = t->tm_wday;
  r->tm_yday  = t->tm_yday;
  r->tm_isdst = t->tm_isdst;
  return r;
}

struct tm *ap_localtime (const time_t *tp) {
  struct tm *result;
  g_static_mutex_lock (&time_mutex);
  result = ap_tm_copy (localtime (tp));
  g_static_mutex_unlock (&time_mutex);
  return result;
}

struct tm *ap_gmtime (const time_t *tp) {
  struct tm *result;
  g_static_mutex_lock (&time_mutex);
  result = ap_tm_copy (gmtime (tp));
  g_static_mutex_unlock (&time_mutex);
  return result;
}

/* Reads from fortune-style file and returns GList of each quote */
static void fortune_helper (GString *s, gchar *data, gboolean escape_html) {
  if (*data == '\n') { 
    g_string_append_printf (s, "<br>");
    return;
  }

  if (escape_html) {
    switch (*data) {
      case '"': g_string_append_printf (s, "&quot;"); return;
      case '&': g_string_append_printf (s, "&amp;");  return;
      case '<': g_string_append_printf (s, "&lt;");   return;
      case '>': g_string_append_printf (s, "&gt;");   return;
    }
  }

  g_string_append_unichar (s, g_utf8_get_char (data));
}

GList *read_fortune_file (const char *filename, gboolean escape_html)
{
  int state;
  gchar *raw_data, *raw_data_start;
  gchar *converted, *text;
  GList *quotes = NULL;
  GString *cur_quote;
  
  if (!g_file_test (filename, G_FILE_TEST_EXISTS)) {
    return NULL;
  }

  if (!g_file_get_contents (filename, &text, NULL, NULL)) {
    return NULL;
  } 

  converted = purple_utf8_try_convert (text);
  if (converted != NULL) {
    g_free (text);
    text = converted;
  }

  raw_data_start = raw_data = purple_utf8_salvage (text);
  g_free (text);

  purple_str_strip_char (raw_data, '\r');

  /* Modeling the parser as a finite state machine */
  state = 0;
  cur_quote = g_string_new ("");
  while (*raw_data) {
    switch (state) {
      /* State after newline (potential quote) */
      case 1:
        if (*raw_data == '%') {  // Found it
          quotes = g_list_append (quotes, strdup (cur_quote->str));
          g_string_truncate (cur_quote, 0);
          state = 2; 
        } else {
          state = 0;
          g_string_append_printf (cur_quote, "<br>");
          fortune_helper (cur_quote, raw_data, escape_html);
        }
        break; 
  
      /* State after end of a quote */
      case 2:
        if (*raw_data != '\n' && *raw_data != '%') {
          state = 0;
          fortune_helper (cur_quote, raw_data, escape_html);
        }
        break;
      /* General state */
      default:
        if (*raw_data == '\n') {
          state = 1; 
        } else {
          fortune_helper (cur_quote, raw_data, escape_html);
        }
        break;
    }

    raw_data = g_utf8_next_char (raw_data);
  }

  if (strlen (cur_quote->str) > 0) {
    quotes = g_list_append (quotes, strdup (cur_quote->str));
  }

  g_string_free (cur_quote, TRUE);
  free (raw_data_start);
  return quotes;
}

/* Returns 1 if a pattern is found at the start of a string */
int match_start (const char *text, const char *pattern)
{
  while (*pattern) {
    if (!*text || *pattern++ != *text++) 
      return 0;
  }
  return 1;
}

/* Free's a GList as well as the internal contents */
void free_string_list (GList *list)
{
  GList *node = list;

  while (node) {
    free (node->data);
    node = node->next;
  }

  g_list_free (list);
}

/* Check if string is in GList */
gboolean string_list_find (GList *lst, const char *data)
{
  while (lst) {
    if (!strcmp (data, (char *) lst->data)) {
      return TRUE;
    }
    lst = lst->next;
  }

  return FALSE;
}

/* Prints out debug messages with repetitive formatting completed */
static void auto_debug_helper (
  PurpleDebugLevel level, const char *category, const char *message) 
{
  GString *s;

  if (message == NULL) 
    message = "NULL";

  s = g_string_new ("");
  g_string_printf (s, "%s: %s\n", category, message); 
  purple_debug (level, "autoprofile", s->str);
  g_string_free (s, TRUE);
}

void ap_debug (const char *category, const char *message) {
  auto_debug_helper (PURPLE_DEBUG_INFO, category, message);
}

void ap_debug_misc (const char *category, const char *message) {
  auto_debug_helper (PURPLE_DEBUG_MISC, category, message);
}

void ap_debug_warn (const char *category, const char *message) {
  auto_debug_helper (PURPLE_DEBUG_WARNING, category, message);
}

void ap_debug_error (const char *category, const char *message) {
  auto_debug_helper (PURPLE_DEBUG_ERROR, category, message);
}


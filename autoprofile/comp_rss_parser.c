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
#include "comp_rss.h"
#include "utility.h"

#include <string.h>

static gboolean parsing_rss = FALSE;
static gboolean parsing_item = FALSE;

static gboolean item_title = FALSE;
static gboolean item_link = FALSE;
static gboolean item_description = FALSE;
static gboolean item_comments = FALSE;
static gboolean item_pubdate = FALSE;

/* Get URL of RSS feed */
static char *get_url (struct widget *w)
{
  int type;
  char *ret;
  const char *username;
  GString *s;  

  type = ap_prefs_get_int (w, "type");
  s = g_string_new ("");

  switch (type) {
    case RSS_LIVEJOURNAL:
      username = ap_prefs_get_string (w, "username");
      g_string_append_printf (s, 
        "http://www.livejournal.com/users/%s/data/rss", username);
    break;
    case RSS_XANGA:
      username = ap_prefs_get_string (w, "username");
      g_string_append_printf (s, "http://www.xanga.com/%s/rss", username);
    break;
    case RSS_2:
      g_string_append_printf (s, "%s", ap_prefs_get_string (w, "location"));
    break;
    default:
    break;
  }

  ret = s->str;
  g_string_free (s, FALSE);
  return ret;
}

/* Utility functions */
static void free_entries (struct widget *w) {
  GList *tmp, *entries;
  struct rss_entry *e;

  entries = (GList *) g_hash_table_lookup (rss_entries, w);

  while (entries) {
    e = (struct rss_entry *) entries->data;
    if (e->title)
      free (e->title);
    if (e->entry)
      free (e->entry);
    if (e->url)
      free (e->url);
    if (e->comments)
      free (e->comments);
    if (e->t)
      free (e->t);
  
    free (e);
    tmp = entries;
    entries = entries->next;
    g_list_free_1 (tmp);
  }
  g_hash_table_replace (rss_entries, w, NULL);
}

/* Date parsing functions */
static struct tm *parse_date_rfc822 (const char *date_string)
{
  time_t t, gmt_time, local_time;
  struct tm *ret, *result;
  
  local_time = time(NULL);
  gmt_time = time(NULL);
  gmt_time = mktime(gmtime(&gmt_time));  

  // TODO: Change this to GDate
  
  t = rfc_parse_date_time (date_string);
 // if (rfc_parse_was_gmt ()) {
    // FIXME: Handle time zones 
 // } else {
    ret = (struct tm *) malloc (sizeof (struct tm));
    result = localtime(&t);
    ret->tm_sec = result->tm_sec;
    ret->tm_min = result->tm_min;
    ret->tm_hour = result->tm_hour;
    ret->tm_mday = result->tm_mday;
    ret->tm_mon = result->tm_mon;
    ret->tm_year = result->tm_year;
  //}

  return ret;
}

/* XML Parser Callbacks */
static void start_element_handler (GMarkupParseContext *context,
                                   const gchar *element_name,
                                   const gchar **attribuate_names,
                                   const gchar **attribute_values,
                                   gpointer data, GError **error)
{
  struct rss_entry *e;
  GList *entries;
  struct widget *w = (struct widget *) data;

  //printf ("start:%s\n", element_name);
 
  if (!parsing_rss && !strcmp (element_name, "rss"))
    parsing_rss = TRUE;

  else if (parsing_rss && !parsing_item && 
      !strcmp (element_name, "item")) {
    parsing_item = TRUE;
    e = (struct rss_entry *) malloc (sizeof(struct rss_entry));
    entries = (GList *) g_hash_table_lookup (rss_entries, w);
    entries = g_list_prepend (entries, e);
    g_hash_table_replace (rss_entries, w, entries);
    e->t = NULL;
    e->title = NULL;
    e->entry = NULL;
    e->url = NULL;
    e->comments = NULL;
  }

  else if (parsing_item && !strcmp (element_name, "title"))
    item_title = TRUE;
  else if (parsing_item && !strcmp (element_name, "link"))
    item_link = TRUE;
  else if (parsing_item && !strcmp (element_name, "description"))
    item_description = TRUE;
  else if (parsing_item && !strcmp (element_name, "comments"))
    item_comments = TRUE;
  else if (parsing_item && !strcmp (element_name, "pubDate"))
    item_pubdate = TRUE;
}

static void end_element_handler (GMarkupParseContext *context,
                                 const gchar *element_name,
                                 gpointer data, GError **error)
{
  struct widget *w = (struct widget *) w;
  //printf ("end:%s\n", element_name);

  if (!strcmp (element_name, "rss"))
    parsing_rss = FALSE;
  else if (!strcmp (element_name, "item")) 
    parsing_item = FALSE;
    
  else if (!strcmp (element_name, "title"))
    item_title = FALSE;
  else if (!strcmp (element_name, "link"))
    item_link = FALSE;
  else if (!strcmp (element_name, "description"))
    item_description = FALSE;
  else if (!strcmp (element_name, "comments"))
    item_comments = FALSE;
  else if (!strcmp (element_name, "pubDate"))
    item_pubdate = FALSE;
}

static void text_handler (GMarkupParseContext *context,
                          const gchar *text, gsize text_len,
                          gpointer data, GError **error)
{
  struct rss_entry *e;
  GList *entries;
  struct widget *w = (struct widget *) data;

  entries = (GList *) g_hash_table_lookup (rss_entries, w);

  if (entries == NULL) {
    return;
  }

  e = (struct rss_entry *) entries->data;

  if (item_link) {
    if (e->url) {
      free (e->url);
    }
    e->url = g_strdup (text);
  }
    
  else if (item_description) {
    if (e->entry) {
      free (e->entry);
    }
    e->entry = purple_unescape_html (text);

    // If there is a standard format for Xanga titles (there really isn't)
    // it will probably be devised from the actual content.  Will be placed
    // here if there is proven demand.
  }

  else if (item_comments) {
    if (e->comments) {
      free (e->comments);
    }
    e->comments = g_strdup (text);
  }

  else if (item_title) {
    if (e->title) {
      free (e->title);
    }
    e->title = g_strdup (text);
  }

  else if (item_pubdate) {
    if (e->t) {
      free (e->t);
    }
    e->t = parse_date_rfc822 (text);
  }
}

/* Final parser variable */
GMarkupParser rss_parser =
{
  start_element_handler,
  end_element_handler,
  text_handler,
  NULL,
  NULL
};

/* Callback for HTTP data fetcher */
static void url_callback (PurpleUtilFetchUrlData *url_data, gpointer data,
		const char *text, size_t size, const gchar *error_message)
{
  GMarkupParseContext *context;
  gchar *filtered_text = NULL, *convert = NULL, *next = NULL;
  GError *err = NULL;
  GList *entries; 
  struct widget *w = (struct widget *) data; 

  /* Make sure URL exists/connected to Internet */
  if (text == NULL) {
    ap_debug ("rss", "error, unable to fetch page via internet");
    return;
  }

  parsing_rss = FALSE;
  parsing_item = FALSE;

  item_title = FALSE;
  item_link = FALSE;
  item_description = FALSE;
  item_comments = FALSE;
  item_pubdate = FALSE;

  g_static_mutex_lock (&rss_mutex);

  free_entries (w);

  // Sanity checking
  filtered_text = purple_utf8_salvage (text);

  *convert = purple_utf8_try_convert ("<");
  *next = g_utf8_strchr (filtered_text, 10, g_utf8_get_char (convert));
  free (convert);

  if (next == NULL) {
    free (filtered_text);
    // TODO: error out
    g_static_mutex_unlock (&rss_mutex);
    return;
  }

  if (ap_prefs_get_int (w, "type") == RSS_XANGA) { 
    parse_xanga_rss (w, filtered_text);
    entries = (GList *) g_hash_table_lookup (rss_entries, w);
    entries = g_list_reverse (entries);
    g_hash_table_replace (rss_entries, w, entries);
    g_static_mutex_unlock (&rss_mutex);
    free (filtered_text);
    return;
  }

  context = g_markup_parse_context_new (&rss_parser, 0, w, NULL);

  if (!g_markup_parse_context_parse (context, next, size, &err)) {
    g_markup_parse_context_free (context);
    ap_debug ("rss", "error, unable to start parser");
    ap_debug ("rss", err->message); 
    free (filtered_text);
    return;
  }
  
  if (!g_markup_parse_context_end_parse (context, &err)) {
    g_markup_parse_context_free (context);
    ap_debug ("rss", "error, unable to end parser");
    free (filtered_text);
    return;
  }
  
  g_markup_parse_context_free (context);

  entries = (GList *) g_hash_table_lookup (rss_entries, w);
  entries = g_list_reverse (entries);
  g_hash_table_replace (rss_entries, w, entries);
  g_static_mutex_unlock (&rss_mutex);

  free (filtered_text);
}

void parse_rss (struct widget *w)
{
  char *url;

  url = get_url (w);
  if (strcmp (url, "") != 0) {
    purple_util_fetch_url (url, TRUE, NULL, FALSE, url_callback, w);
  }
  free (url);
}



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

#include <stdlib.h>

#include <time.h>
#include <string.h>
#include <stdio.h>

#include "autoprofile.h"
#include "comp_rss.h"
#include <glib.h>

static gchar *convert_char;


static gchar *find_next_tag (gchar *text) {
  *convert_char = '<';
  return g_utf8_strchr (text, -1, g_utf8_get_char (convert_char));
}

static gchar *find_end_of_tag (gchar *text) {
  *convert_char = '>';
  return g_utf8_strchr (text, -1, g_utf8_get_char (convert_char));
}

static gboolean starts_with (gchar *text, gchar letter) {
  *convert_char = letter;
  return (g_utf8_strchr (text, 1, g_utf8_get_char (convert_char)) == text);
}

void parse_xanga_rss (struct widget *w, gchar *text) {
  gchar *next_tag, *end_prev_tag;
  gchar *tag_first_char, *tag_second_char;
  gboolean is_item;

  convert_char = (gchar *) malloc (sizeof(gchar) * 2);
  *(convert_char+1) = '\0';

  end_prev_tag = text;
  is_item = FALSE;
  
  rss_parser.start_element (NULL, "rss", NULL, NULL, w, NULL);

  while ((next_tag = find_next_tag (end_prev_tag)) != NULL) {
    tag_first_char = g_utf8_next_char (next_tag);
    tag_second_char = g_utf8_next_char (tag_first_char);

    if (is_item) {
      if (starts_with (tag_first_char, 't')) rss_parser.start_element (
        NULL, "title", NULL, NULL, w, NULL);
      else if (starts_with (tag_first_char, 'l')) rss_parser.start_element (
        NULL, "link", NULL, NULL, w, NULL);
      else if (starts_with (tag_first_char, 'p')) rss_parser.start_element (
        NULL, "pubDate", NULL, NULL, w, NULL);
      else if (starts_with (tag_first_char, 'd')) rss_parser.start_element (
        NULL, "description", NULL, NULL, w, NULL);
      else if (starts_with (tag_first_char, 'c')) rss_parser.start_element (
        NULL, "comments", NULL, NULL, w, NULL);
      else if (starts_with (tag_first_char, '/')) {
        *next_tag = '\0';
        rss_parser.text (NULL, end_prev_tag, -1, w, NULL);

        if (starts_with (tag_second_char, 't')) 
          rss_parser.end_element (NULL, "title", w, NULL);
        else if (starts_with (tag_second_char, 'l')) 
          rss_parser.end_element (NULL, "link", w, NULL);
        else if (starts_with (tag_second_char, 'p')) 
          rss_parser.end_element (NULL, "pubDate", w, NULL);
        else if (starts_with (tag_second_char, 'd')) 
          rss_parser.end_element (NULL, "description", w, NULL);
        else if (starts_with (tag_second_char, 'c')) 
          rss_parser.end_element (NULL, "comments", w, NULL);
        else if (starts_with (tag_second_char, 'i')) {
          rss_parser.end_element (NULL, "item", w, NULL);
          is_item = FALSE;
        } else {
          // TODO: WARN USER IN THIS CASE
        }
      }
    } else {
      if (starts_with (tag_first_char, 'i') &&
          starts_with (tag_second_char, 't')) {
        is_item = TRUE; 
        rss_parser.start_element (NULL, "item", NULL, NULL, w, NULL);
      }
    }

    if ((next_tag = find_end_of_tag (tag_first_char)) == NULL) {
      // TODO: NOTIFY USER THAT WE REACHED END
      return;
    }
    end_prev_tag = g_utf8_next_char (next_tag);
  }

  free (convert_char);
}


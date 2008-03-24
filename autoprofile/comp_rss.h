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

#include "component.h"
#include "time.h"

#define MAX_USERNAME_LENGTH 1000

struct rss_entry {
  struct tm *t;
  char *title;
  char *entry;
  char *url;
  char *comments;
};

typedef enum
{
  RSS_UNKNOWN = -1,
  RSS_XANGA,
  RSS_LIVEJOURNAL,
  RSS_2
} RSS_TYPE;

extern GHashTable *rss_entries;
extern GStaticMutex rss_mutex;

void parse_rss (struct widget *);
void parse_xanga_rss (struct widget *, gchar *);
GMarkupParser rss_parser;



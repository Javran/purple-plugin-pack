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

struct tm *ap_localtime (const time_t *);
struct tm *ap_gmtime (const time_t *);

GList *read_fortune_file (const char *, gboolean);
int match_start (const char *, const char *);
void free_string_list (GList *);
gboolean string_list_find (GList *, const char *);

void ap_debug (const char *, const char *);
void ap_debug_misc (const char *, const char *);
void ap_debug_warn (const char *, const char *);
void ap_debug_error (const char *, const char *);

/* RFC 822 Date/Time */
#include <time.h>
time_t rfc_parse_date_time (const char *data);
int rfc_parse_was_gmt ();


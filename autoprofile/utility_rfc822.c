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

#include <time.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

static struct tm parsed_datetime;
static int parsed_gmttime = 0;

/* Strip leading whitespace */
static const char* rfc_parse_whitespace (const char *data) {
  while (*data && isspace (*data))
    data++;
  return data;
}

/* Strip leading digits */
static const char* rfc_parse_num (const char *data) {
  while (*data && isdigit (*data))
    data++;
  return data;
}

/* Strip leading whitespace and digits */
static const char* rfc_parse_whitespace_num (const char *data) {
  while (*data && (isspace (*data) || isdigit (*data)))
    data++;
  return data;
}


static const char* rfc_parse_date (const char *data) {
  char month[4];
  int day = 0;
  int year = 0;
  int monthnum = 0;
  
  sscanf (data, "%d", &day);
  data = rfc_parse_whitespace_num (data);
  sscanf (data, "%s", month);

  if (!strcmp (month, "Jan")) { monthnum = 0; } else
  if (!strcmp (month, "Feb")) { monthnum = 1; } else
  if (!strcmp (month, "Mar")) { monthnum = 2; } else
  if (!strcmp (month, "Apr")) { monthnum = 3; } else
  if (!strcmp (month, "May")) { monthnum = 4; } else
  if (!strcmp (month, "Jun")) { monthnum = 5; } else
  if (!strcmp (month, "Jul")) { monthnum = 6; } else
  if (!strcmp (month, "Aug")) { monthnum = 7; } else
  if (!strcmp (month, "Sep")) { monthnum = 8; } else
  if (!strcmp (month, "Oct")) { monthnum = 9; } else
  if (!strcmp (month, "Nov")) { monthnum = 10; } else
  if (!strcmp (month, "Dec")) { monthnum = 11; } 
  
  data += 3;  
  sscanf (data, "%d", &year);
  data = rfc_parse_whitespace (data);
  data = rfc_parse_num (data);

  if (year < 50) {
    year += 100;
  } else if (year > 100) {
    year -= 1900;
  }
  
  /* Set the values */
  parsed_datetime.tm_mday = day;
  parsed_datetime.tm_mon = monthnum; 
  parsed_datetime.tm_year = year;

  return data;
}

static const char* rfc_parse_hour (const char *data) {
  int hour = 0;
  int minutes = 0;
  int seconds = 0;
  
  sscanf (data, "%d", &hour);
  data = strchr (data, ':');
  sscanf (++data, "%d", &minutes);
 
  if (strchr (data, ':')) {
    data = strchr (data, ':') + 1;
    sscanf (data, "%d", &seconds);
    data = rfc_parse_whitespace_num (data);
  }

  parsed_datetime.tm_hour = hour;
  parsed_datetime.tm_min = minutes;
  parsed_datetime.tm_sec = seconds;
  
  return data;
}

static const char *rfc_parse_zone (const char *data) {
  if (strstr (data, "GMT"))
    parsed_gmttime = 1;
  else
    parsed_gmttime = 0;

  return data;
}

static const char* rfc_parse_time (const char *data) {
  data = rfc_parse_hour (data);
  data = rfc_parse_zone (data);
  return data;
}

static const char* rfc_parse_day (const char *data) {
  return strchr (data, ',') + 1;
}

int rfc_parse_was_gmt () {
  return parsed_gmttime;
}

time_t rfc_parse_date_time (const char *data) {
  time_t result;
  /* Initialize values */
  parsed_datetime.tm_sec = 0;
  parsed_datetime.tm_min = 0;
  parsed_datetime.tm_hour = 0;
  parsed_datetime.tm_mday = 0;
  parsed_datetime.tm_mon = 0;
  parsed_datetime.tm_year = 0;
  parsed_datetime.tm_isdst = -1;
  
  data = rfc_parse_whitespace (data);
  if (isalpha (*data)) {
    data = rfc_parse_day (data);
  }

  data = rfc_parse_date (data);
  data = rfc_parse_time (data);

  result = mktime(&parsed_datetime);

#ifndef __BSD_VISIBLE
  if (rfc_parse_was_gmt ())
    result -= timezone;
#endif

  return result;
}

/* DEBUGGING 

int main () { 

  struct tm *x = rfc_parse_date_time ("Mon, 06 Jun 2005 20:24:18 GMT");
  
  printf ("Sec: %d\n", x->tm_sec);
  printf ("Min: %d\n", x->tm_min);
  printf ("Hour: %d\n", x->tm_hour);
  printf ("Day: %d\n", x->tm_mday);
  printf ("Month: %d\n", x->tm_mon);
  printf ("Year: %d\n", x->tm_year);

  printf ("GMT: %d\n", parsed_gmttime); 
  
  return 0;
}

*/


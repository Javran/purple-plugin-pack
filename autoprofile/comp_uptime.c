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

#include "../common/pp_internal.h"

#include "component.h"
#include "utility.h"

#include <string.h>

/*---------- UPTIME: Display the computer uptime --*/
char *uptime_generate (struct widget *w) {
  gboolean exec;
  char *out, *line, *working;
  char *p_character, *colon_character, *comma_character, *m_character;
  GError *return_error;

  line = N_("uptime");

  exec = g_spawn_command_line_sync (line, 
    &out, NULL, NULL, &return_error);
  /* Parse the output */
  if (exec) {
    /* Buffer length for safety */
    working = (char *)malloc (strlen (out)+7+8+8+1);
    strcpy (working, "Uptime:"); 
    /* Break into minutes, hours, and everything else */
    p_character = strchr (out, 'p');
    m_character = strchr (p_character, 'm');

    /* Uptime format including "pm" */
    if (m_character != NULL && m_character == p_character + 1) {
      p_character = strchr (m_character, 'p');
      m_character = strchr (p_character, 'm');
    }

    /* Uptime if < 1 hour */
    if (m_character != NULL && *(m_character+1) == 'i') {
      *m_character = '\0';
      p_character++;
      strcat (working, p_character);
      strcat (working, "minutes");

    /* General uptime */
    } else { 
      colon_character = strchr (p_character, ':');
      comma_character = strchr (colon_character, ',');
      p_character++;
      *colon_character++ = '\0';
      *comma_character = '\0';
      /* Yank it all together */
      strcat (working, p_character);
      strcat (working, " hours, ");
      strcat (working, colon_character); 
      strcat (working, " minutes");
    }
  
    free (out); 
    return working;
  } else {
    ap_debug ("uptime", "command failed to execute");
    return g_strdup(_("[ERROR: failed to execute uptime command]"));
    return NULL;
  }
}

struct component uptime =
{
  N_("Uptime"),
  N_("Show how long your computer has been running"),
  "Uptime",
  &uptime_generate,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};



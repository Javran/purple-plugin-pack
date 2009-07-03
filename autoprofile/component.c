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

#include <string.h>

static GList *components = NULL;

static GList *get_components ();

void ap_component_start () {
  if (components) g_list_free (components);
  components = get_components ();

  ap_widget_start ();
}
void ap_component_finish () {
  ap_widget_finish ();

  g_list_free (components);
  components = NULL;
}

GList *ap_component_get_components () {
  return components;
}

struct component *ap_component_get_component (const gchar *identifier) {
  GList *comps;
  struct component *cur_comp;

  for (comps = components; comps != NULL; comps = comps->next) {
    cur_comp = (struct component *) comps->data;

    if (!strcmp (cur_comp->identifier, identifier))
      return cur_comp;
  }

  return NULL;
}

static GList *get_components ()
{
  GList *ret = NULL;
  /* Add each listed component */

  /*
  ret = g_list_append (ret, &logstats);
  */

  ret = g_list_append (ret, &text);
  ret = g_list_append (ret, &quotation);
  ret = g_list_append (ret, &rss);
  ret = g_list_append (ret, &timestamp);
  ret = g_list_append (ret, &http);  
  ret = g_list_append (ret, &count);

  #ifndef _WIN32
  ret = g_list_append (ret, &executable);
  ret = g_list_append (ret, &uptime);
  #endif

  /* Return */
  return ret;
}


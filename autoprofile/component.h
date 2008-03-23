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

#ifndef _AP_COMPONENT_H_
#define _AP_COMPONENT_H_

#include "widget.h"

#include "sizes.h"

#include "prefs.h"
#include "notify.h"
#include "util.h"

#include "pidgin.h"
#include "gtkutils.h"

/* A component is composed of code that generates some sort of content,
   and a widget is a specific _instance_ of a component */
struct widget;

struct component {
  char *name;                   
  char *description;
  char *identifier;
  char *(*generate)(struct widget *);
  void (*init_pref)(struct widget *);
  void (*load)(struct widget *);
  void (*unload)(struct widget *);
  gboolean (*has_content_changed)(struct widget *);
  GtkWidget *(*pref_menu)(struct widget *);
};

void ap_component_start ();
void ap_component_finish ();

GList *ap_component_get_components ();
struct component *ap_component_get_component (const gchar *);

/* TEMP
extern struct component logstats;
*/

extern struct component count;
extern struct component executable;
extern struct component http;
extern struct component quotation;
extern struct component rss;
extern struct component text;
extern struct component timestamp;
extern struct component uptime;

#endif /* _AP_COMPONENT_H_ */

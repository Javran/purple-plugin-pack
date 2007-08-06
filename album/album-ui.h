/*
 * Album (Buddy Icon Archiver)
 *
 * Copyright (C) 2005-2006, Sadrul Habib Chowdhury <imadil@gmail.com>
 * Copyright (C) 2005-2006, Richard Laager <rlaager@users.sf.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
#ifndef _ALBUM_UI_H_
#define _ALBUM_UI_H_

#include "album.h"

#include <blist.h>

#define PREF_WINDOW_HEIGHT  PREF_PREFIX "/window_height"
#define PREF_WINDOW_WIDTH   PREF_PREFIX "/window_width"
#define PREF_ICON_SIZE      PREF_PREFIX "/icon_size"

extern GHashTable *buddy_windows;

guint icon_viewer_hash(gconstpointer data);

gboolean icon_viewer_equal(gconstpointer y, gconstpointer z);

GList *album_get_plugin_actions(PurplePlugin *plugin, gpointer data);

void album_blist_node_menu_cb(PurpleBlistNode *node, GList **menu);

void album_update_runtime(PurpleBuddy *buddy, gpointer data);

#endif /* _ALBUM_UI_H_ */

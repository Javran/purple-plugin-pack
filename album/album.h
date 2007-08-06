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
#ifndef _ALBUM_H_
#define _ALBUM_H_

#include "../common/pp_internal.h"

#include <account.h>

#define PLUGIN_STATIC_NAME "album"
#define PLUGIN_ID          "gtk-rlaager-" PLUGIN_STATIC_NAME
#define PREF_PREFIX        "/plugins/" PLUGIN_ID

char *album_buddy_icon_get_dir(PurpleAccount *account, const char *name);

#endif /* _ALBUM_H_ */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef IGNORANCE_DENIZEN_H
#define IGNORANCE_DENIZEN_H

#include <string.h>

#include "ignorance.h"

typedef struct ignorance_denizen{
	gchar *name;
	gchar *last_message;
	gint repeats;
} ignorance_denizen;

ignorance_denizen* ignorance_denizen_new(const gchar *newname);

void ignorance_denizen_free(ignorance_denizen *denizen);

gchar* ignorance_denizen_get_name(ignorance_denizen *id);

gchar* ignorance_denizen_get_last_message(ignorance_denizen *id);

gint ignorance_denizen_get_repeats(ignorance_denizen *id);

gint ignorance_denizen_set_message(ignorance_denizen *id, const gchar *message);

#endif

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

#ifndef IGNORANCE_H
#define IGNORANCE_H

#ifdef HAVE_CONFIG_H
# include "../pp_config.h"
#endif

#include "../common/pp_internal.h"

#include "ignorance_level.h"

#define IGNORANCE_APPLY_CHAT	1
#define IGNORANCE_APPLY_IM		2
#define IGNORANCE_APPLY_USER	4
#define IGNORANCE_APPLY_HOST	8
#define IGNORANCE_APPLY_ENTERLEAVE 16
#define IGNORANCE_APPLY_INVITE	32

ignorance_level*	ignorance_get_level_name(const GString *levelname);

gboolean ignorance_add_level(ignorance_level *level);

gboolean ignorance_remove_level(const GString *levelname);

gint ignorance_get_new_level_index();

gboolean save_conf();

#endif

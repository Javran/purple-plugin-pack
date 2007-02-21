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

#ifndef IGNORANCE_VIOLATION_H
#define IGNORANCE_VIOLATION_H

#include "ignorance.h"

typedef struct ignorance_violation {
	gint type;
	gchar *value;
} ignorance_violation;

ignorance_violation* ignorance_violation_new();
ignorance_violation* ignorance_violation_newp(gint newtype, const gchar *newvalue);
void ignorance_violation_free(ignorance_violation *iv);
void ignorance_violation_free_g(gpointer iv, gpointer user_data);

#endif

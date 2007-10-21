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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301, USA.
 */

#include "ignorance_violation.h"
#include "ignorance_internal.h"

ignorance_violation* ignorance_violation_new() {
	return ignorance_violation_newp(IGNORANCE_FLAG_MESSAGE,"");
}

ignorance_violation* ignorance_violation_newp(gint newtype,
											  const gchar *newvalue){
	ignorance_violation *iv=(ignorance_violation*)g_malloc(sizeof(ignorance_violation));

	if(iv){
		iv->type=newtype;
		iv->value=g_strdup(newvalue);
	}

	return iv;
}

void ignorance_violation_free(ignorance_violation *iv) {
	if(iv){
		g_free(iv->value);
		g_free(iv);
	}
}

void ignorance_violation_free_g(gpointer iv, gpointer user_data) {
	ignorance_violation_free((ignorance_violation*)iv);
}

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

#include "ignorance_denizen.h"
#include "ignorance_internal.h"

ignorance_denizen* ignorance_denizen_new(const gchar *newname) {
	ignorance_denizen *id=(ignorance_denizen*)g_malloc(sizeof(ignorance_denizen));

	id->name=g_strdup(newname);
	id->last_message=g_strdup("");
	id->repeats=0;

	return id;
}

void ignorance_denizen_free(ignorance_denizen *id) {
	g_free(id->name);
	g_free(id->last_message);
	g_free(id);
}

gchar *ignorance_denizen_get_name(ignorance_denizen *id) {
	return id->name;
}

gchar *ignorance_denizen_get_last_message(ignorance_denizen *id) {
	return id->last_message;
}

gint ignorance_denizen_get_repeats(ignorance_denizen *id) {
	return id->repeats;
}

gint ignorance_denizen_set_message(ignorance_denizen *id, const gchar *message) {
	if(!strcasecmp(id->last_message,message)){
		purple_debug_info("ignorance","Got repeat %d for message %s\n",
						id->repeats+1,message);
		id->repeats++;
	} else {
		purple_debug_info("ignorance","New message %s replacing old message %s",
						message,id->last_message);
		g_free(id->last_message);
		id->last_message=g_strdup(message);
		id->repeats=0;
	}

	return id->repeats;
}

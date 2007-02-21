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

#ifndef IGNORANCE_LEVEL_H
#define IGNORANCE_LEVEL_H

#include "ignorance_rule.h"

/*
 * describes an ignorance level
 * 
 * index is an index for keeping track
 * of levels' relative positions
 * the idea is for 0 to be the "default" level,
 * >0 to be "better" levels, and <0 to be "worse"
 *
 * allow_passthrough flags whether a user
 * is allowed to be "passed through" this level
 * to the next consecutive level, as in:
 * if someone on my friends list does something
 * that flags 12 of my rules, do I send them 
 * straight to /dev/null, or just 
 * bump them down a level?
 *
 * name - user's name for the level
 *
 * denizens - list of users in this level
 * I may get rid of this in favor of a more global
 * userlist
 *
 * rules - list of rules for this level
 */
typedef struct ignorance_level{
	GString *name;
	/*GList		*denizens;*/
	GHashTable *denizens_hash;
	GPtrArray *rules;
} ignorance_level;

ignorance_level* ignorance_level_new();
void ignorance_level_free(ignorance_level *il);
void ignorance_level_free_g(gpointer il,gpointer user_data);

gboolean ignorance_level_add_rule(ignorance_level *level,ignorance_rule *rule);

ignorance_rule* ignorance_level_get_rule(ignorance_level *level,
										 const GString *rulename);

gboolean ignorance_level_remove_rule(ignorance_level *level,
									 const GString *rulename);

gboolean ignorance_level_add_denizen(ignorance_level *level,
									 const GString *username);

gboolean ignorance_level_add_denizen_fast(ignorance_level *level,
										  const GString *username);

gboolean ignorance_level_has_denizen(ignorance_level *level,
									 const GString *username);

#ifdef HAVE_REGEX_H
gboolean ignorance_level_has_denizen_regex(ignorance_level *level,
										   const gchar *regex, GList **denizens);
#endif

gboolean ignorance_level_remove_denizen(ignorance_level *level,
										const GString *username);

/*
 * Determines whether a string violates one of 
 * the rules defined in an ignorance level
 *
 * il is the ignorance level
 * text is the possibly offending string
 * flags are the rule flags to match
 * returns score
 */
gint ignorance_level_rulecheck(ignorance_level *il,const GString *username,
							   const GString *text, gint flags,
							   GList **violations);

ignorance_level* ignorance_level_read(const gchar *lvltext);

gboolean ignorance_level_write(ignorance_level *level,FILE *f);

#endif

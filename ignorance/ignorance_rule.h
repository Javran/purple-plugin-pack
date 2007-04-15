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

#ifndef IGNORANCE_RULE_H
#define IGNORANCE_RULE_H

#ifdef HAVE_CONFIG_H
# include "../pp_config.h"
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <glib.h>
#include <glib/gprintf.h>

#include "debug.h"

#define IGNORANCE_RULE_INVALID 0
#define IGNORANCE_RULE_MINVALID INT_MAX

#define IGNORANCE_RULE_SIMPLETEXT 1
#define IGNORANCE_RULE_SIMPLETEXT_NUMTOKENS 6

#ifdef HAVE_REGEX_H
#include <regex.h>
#define IGNORANCE_RULE_REGEX 2
#define IGNORANCE_RULE_REGEX_NUMTOKENS 6
#endif

#define IGNORANCE_RULE_REPEAT 4
#define IGNORANCE_RULE_REPEAT_NUMTOKENS 6

#define IGNORANCE_FLAG_FILTER	1
#define IGNORANCE_FLAG_IGNORE	2
#define IGNORANCE_FLAG_MESSAGE  4
#define IGNORANCE_FLAG_EXECUTE	8
#define IGNORANCE_FLAG_SOUND	16

/*
 * describes an ignorance rule
 *
 * name - user's name for the rule
 * type - one of the ruletypes defined within this struct
 *
 * value - the actual value of the rule
 * could be a string/regex, integer, other
 * depending on the rule type
 *
 * score - an arbitrary (user-assigned) number
 * that determines the severity of the rule
 */
typedef struct ignorance_rule {
	GString *name;
	gint type;
	gchar *value;
	gint score;
	gint flags;
	gboolean enabled;
	gchar *message,
		*command,
		*sound;
} ignorance_rule;

ignorance_rule* ignorance_rule_new();

ignorance_rule* ignorance_rule_newp(const GString *name, gint type,
									const gchar *value, gint score, gint flags,
									gboolean enabled, const gchar *message,
									const gchar *sound, const gchar *command);
void ignorance_rule_free(ignorance_rule *ir);
void ignorance_rule_free_g(gpointer ir,gpointer user_data);

gboolean ignorance_rule_has_type(gint type);

/*
 * Determines whether a string violates a rule
 *
 * rule is the rule
 * text is the possibly offending string
 * returns 0/1 boolean
 */
gint ignorance_rule_rulecheck(ignorance_rule *rule, const GString *text,
							  gint flags);

gint simple_text_rulecheck(ignorance_rule *rule, const GString *text);

#ifdef HAVE_REGEX_H
gint regex_rulecheck(ignorance_rule *rule, const GString *text);
#endif

gint repeat_rulecheck(ignorance_rule *rule, gint repeats);

ignorance_rule* ignorance_rule_read(const gchar *ruletext);

/*
 * Writes out an ignorance rule to a file
 *
 * rule is the rule to write
 * f is the file which will be written
 * success/failure returned
 */
gboolean ignorance_rule_write(ignorance_rule *rule, FILE *f);

#endif

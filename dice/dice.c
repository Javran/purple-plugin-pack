/*
 * Adds a command to roll an arbitrary number of dice with an arbitrary 
 * number of sides
 * Copyright (C) 2005 Gary Kramlich <grim@reaperworld.com>
 * Modified and commented 2007-11 by Lucas <reilithion@gmail.com>
 *
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

/* If you can't figure out what this line is for, DON'T TOUCH IT. */
#include "../common/pp_internal.h"

#include <time.h>
#include <stdlib.h>

#include <cmds.h>
#include <conversation.h>
#include <debug.h>
#include <plugin.h>

#define DEFAULT_DICE	2
#define DEFAULT_SIDES	6

#define BOUNDS_CHECK(var, min, min_def, max, max_def) { \
	if((var) < (min)) \
		(var) = (min_def); \
	else if((var) > (max)) \
		(var) = (max_def); \
}

#define ROUND(val) ((gdouble)(val) + 0.5f)

static PurpleCmdId dice_cmd_id = 0;

static gchar *
old_school_roll(gint dice, gint sides) {
	GString *str = g_string_new("");
	gchar *ret = NULL;
	gint c = 0, v = 0;

	BOUNDS_CHECK(dice, 1, 2, 15, 15);
	BOUNDS_CHECK(sides, 2, 2, 999, 999);

	g_string_append_printf(str, "%d %d-sided %s:",
						   dice, sides,
						   (dice == 1) ? "die" : "dice");

	for(c = 0; c < dice; c++) {
		v = rand() % sides + 1;

		g_string_append_printf(str, " %d", v);
	}

	ret = str->str;
	g_string_free(str, FALSE);

	return ret;
}

static inline gboolean
is_dice_notation(const gchar *str) {
	return (g_utf8_strchr(str, -1, 'd') != NULL);
}

static gchar *
dice_notation_roll_helper(const gchar *dn, gint *value) {
	GString *str = g_string_new("");
	gchar *ret = NULL, *ms = NULL;
	gchar op = '\0';
	gint dice = 0, sides = 0, i = 0, t = 0, v = 0;
	gdouble multiplier = 1.0;

	if(!dn || *dn == '\0')
		return NULL;

	/* at this point, all we have is +/- number for our bonus, so we add it to
	 * our value
	 */
	if(!is_dice_notation(dn)) {
		gint bonus = atoi(dn);

		*value += bonus;

		/* the + makes sure we always have a + or - */
		g_string_append_printf(str, "%s %d",
							   (bonus < 0) ? "-" : "+",
							   ABS(bonus));

		ret = str->str;
		g_string_free(str, FALSE);

		return ret;
	}

	/**************************************************************************
	 * Process our block
	 *************************************************************************/
	purple_debug_info("dice", "processing '%s'\n", dn);

	/* get the number of dice */
	dice = atoi(dn);
	BOUNDS_CHECK(dice, 1, 1, 999, 999);

	/* find and move to the character after the d */
	dn = g_utf8_strchr(dn, -1, 'd');
	dn++;

	/* get the number of sides */
	sides = atoi(dn);
	BOUNDS_CHECK(sides, 2, 2, 999, 999);

	/* i've struggled with a better way to determine the next operator, i've 
	 * opted for this.
	 */
	for(t = sides; t > 0; t /= 10) {
		dn++;
		purple_debug_info("dice", "looking for the next operator: %s\n", dn);
	}

	purple_debug_info("dice", "next operator: %s\n", dn);

	/* check if we're multiplying or dividing this block */
	if(*dn == 'x' || *dn == '/') {
		op = *dn;
		dn++;

		multiplier = v = atof(dn);

		ms = g_strdup_printf("%d", (gint)multiplier);

		/* move past our multiplier */
		for(t = v; t > 0; t /= 10) {
			purple_debug_info("dice", "moving past the multiplier: %s\n", dn);
			dn++;
		}

		if(op == '/')
			multiplier = 1 / multiplier;
	}

	purple_debug_info("dice", "d=%d;s=%d;m=%f;\n", dice, sides, multiplier);

	/* calculate and output our block */
	g_string_append_printf(str, " (");
	for(i = 0; i < dice; i++) {
		t = rand() % sides + 1;
		v = ROUND(t * multiplier);

		g_string_append_printf(str, "%s%d", (i > 0) ? " " : "", t);
		
		purple_debug_info("dice", "die %d: %d(%d)\n", i, v, t);

		*value += v;
	}

	g_string_append_printf(str, ")");

	/* if we have a multiplier, we need to output it as well */
	if(multiplier != 1.0)
		g_string_append_printf(str, "%c(%s)", op, ms);

	/* free our string of the multiplier */
	g_free(ms);

	purple_debug_info("dice", "value=%d;str=%s\n", *value, str->str);

	/* we have more in our string, recurse! */
	if(*dn != '\0') {
		gchar *s = dice_notation_roll_helper(dn, value);
		
		if(s)
			str = g_string_append(str, s);

		g_free(s);
	}

	ret = str->str;
	g_string_free(str, FALSE);

	return ret;
}

static gchar *
dice_notation_roll(const gchar *dn) {
	GString *str = g_string_new("");
	gchar *ret = NULL, *normalized = NULL;
	gint value = 0;

	g_string_append_printf(str, "%s:", dn);

	/* normalize the input and process it */
	normalized = g_utf8_strdown(dn, -1);
	g_string_append_printf(str, "%s",
						   dice_notation_roll_helper(normalized, &value));
	g_free(normalized);

	g_string_append_printf(str, " = %d", value);

	ret = str->str;
	g_string_free(str, FALSE);

	return ret;
}

static PurpleCmdRet
roll(PurpleConversation *conv, const gchar *cmd, gchar **args, gchar *error,
	 void *data)
{
	PurpleCmdStatus ret;
	gchar *str = NULL, *newcmd = NULL;

	if(!args[0]) {
		str = old_school_roll(DEFAULT_DICE, DEFAULT_SIDES);
	} else {
		if(is_dice_notation(args[0])) {
			str = dice_notation_roll(args[0]);
		} else {
			gint dice, sides;
			
			dice = atoi(args[0]);
			sides = (args[1]) ? atoi(args[1]) : DEFAULT_SIDES;

			str = old_school_roll(dice, sides);
		}
	}

#if 0
			i = 1;	/* Abuse that iterator!  We're saying "We think this is dice notation!" */
			splitted = g_strsplit(args[0], "d", 2);	/* Split the description into two parts: (1)d(20+5); discard the 'd'. */
			dice = atoi(splitted[0]);	/* We should have the number of dice easily now. */
			if(g_strstr_len(splitted[1], -1, "+") != NULL)	/* If our second half contained a '+' (20+5) */
			{
				resplitted = g_strsplit(splitted[1], "+", 2);	/* Split again: (20)+(5); discard the '+'. */
				sides = atoi(resplitted[0]);	/* Number of sides on the left. */
				bonus += atoi(resplitted[1]);	/* Bonus on the right. */
				g_strfreev(resplitted);			/* Free memory from the split. */
			}
			else if(g_strstr_len(splitted[1], -1, "-") != NULL)	/* If our second half contained a '-' (20-3) */
			{
				resplitted = g_strsplit(splitted[1], "-", 2);	/* Split again: (20)-(3); discard the '-'. */
				sides = atoi(resplitted[0]);	/* Number of sides on the left. */
				bonus -= atoi(resplitted[1]);	/* Penalty on the right. */
				g_strfreev(resplitted);			/* Free memory from the split. */
			}
			else	/* There was neither a '+' nor a '-' in the second half. */
				sides = atoi(splitted[1]);	/* We're assuming it's just a number, then.  Number of sides. */
			g_strfreev(splitted);	/* Free the original split. */
		}
	}

	if(args[1] && i == 0)	/* If there was a second argument, and we care about it (not dice notation) */
		sides = atoi(args[1]);	/* Grab it and make it the number of sides the dice have. */

	str = g_string_new("");
	if(i)	/* Show the output in dice notation format. */
	{
		g_string_append_printf(str, "%dd%d", dice, sides);	/* For example, 1d20 */
		if(bonus > 0)
			g_string_append_printf(str, "+%d", bonus);	/* 1d20+5 */
		else if(bonus < 0)
			g_string_append_printf(str, "%d", bonus);	/* 1d20-3 (saying "-%d" would be redundant, since the '-' gets output with bonus automatically) */
		g_string_append_printf(str, ":");	/* Final colon.  1d20-4: */
	}

	for(i = 0; i < dice; i++)	/* For each die... */
	{
		roll = rand() % sides + 1;	/* Roll, and add bonus. */
		accumulator += roll;			/* Accumulate our rolls */
		g_string_append_printf(str, " %d", roll);	/* Append the result of our roll to our output string. */
	}
	
	if(bonus != 0)	/* If we had a bonus */
	{
		accumulator += bonus;	/* Accumulate our bonus/penalty */
		g_string_append_printf(str, " %s%d = %d", (bonus < 0) ? "penalty " : "bonus +", bonus, accumulator); /* Append our bonus/penalty to the output string */
	}
	else if(dice > 1)	/* Or if we had more than one die */
	{
		g_string_append_printf(str, " = %d", accumulator);	/* Append our accumulator */
	}
#endif

	newcmd = g_strdup_printf("me rolls %s", str);

	ret = purple_cmd_do_command(conv, newcmd, newcmd, &error);

	g_free(str);
	g_free(newcmd);

	return ret;
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	const gchar *help;

	help = _("dice [dice] [sides]:  rolls dice number of sides sided dice OR\n"
			 "dice [XdY+-Z]:  rolls X number of Y sided dice, giving a Z "
			 "bonus/penalty to each.  e.g. 1d20+2");

	dice_cmd_id = purple_cmd_register("dice", "wws", PURPLE_CMD_P_PLUGIN,
									PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT |
									PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS,
									NULL, PURPLE_CMD_FUNC(roll),
									help, NULL);

	/* we only want to seed this off of the seconds since the epoch once.  If
	 * we do it every time, we'll give the same results for each time we
	 * process a roll within the same second.  This is bad because it's not
	 * really random then.
	 */
	srand(time(NULL));

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	purple_cmd_unregister(dice_cmd_id);

	return TRUE;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,	
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,

	"core-plugin_pack-dice",
	NULL,
	PP_VERSION,	
	NULL,
	NULL,
	"Gary Kramlich <grim@reaperworld.com>",	
	PP_WEBSITE,	

	plugin_load,
	plugin_unload,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Dice");
	info.summary = _("Rolls dice in a chat or im");
	info.description = _("Adds a command (/dice) to roll an arbitrary "
						 "number of dice with an arbitrary number of sides. "
						 "Now supports dice notation!  /help dice for details");

}

PURPLE_INIT_PLUGIN(flip, init_plugin, info)

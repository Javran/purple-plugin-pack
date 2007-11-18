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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifdef HAVE_CONFIG_H
# include "../pp_config.h"	/* Purple Plugin Pack common configuration header. */
#endif /* HAVE_CONFIG_H */

#define PURPLE_PLUGINS	/* I get the impression this is necessary somehow... */

#include <glib.h>	/* For glib wrappers of the standard C types and my beloved string functions. */
#include <time.h>	/* For the random number seed. */
#include <stdlib.h>	/* Gratuitous, Ubiquitous, Includimust! */

#include <cmds.h>			/* Must be related to extending Purple's /command set. */
#include <conversation.h>	/* For being able to put stuff in a conversation? */
#include <debug.h>			/* Always a good idea. */
#include <plugin.h>			/* 'Cause this is a plugin, I'd think. */
#include <version.h>		/* Also always a good idea. */

#include "../common/pp_internal.h"	/* No idea what this is for. */

static PurpleCmdId dice_cmd_id = 0;	/* No idea what this does. */

static PurpleCmdRet
roll(PurpleConversation *conv, const gchar *cmd, gchar **args,
	 gchar *error, void *data)
{
	gchar **splitted = NULL, **resplitted = NULL;	/* Variables for splitting up a dice notation string */
	GString *str = NULL;	/* Our output */
	gint dice = 2, sides = 6, bonus = 0, accumulator = 0, roll, i = 0;	/* How many dice, sides for each, the bonus for each, an accumulator to gather the total, our roll each time, and an iterator which I abuse before it's used. */

	srand(time(NULL));	/* Seed our random number generator. */

	if(args[0]) /* If we got an argument */
	{
		if(g_strstr_len(args[0], -1, "d") == NULL) /* There's no 'd' in our string. */
			dice = atoi(args[0]);
		else	/* Process dice notation: xdy+-z */
		{
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

	/*
	 * These next four lines make sure we're rolling a sane
	 * number of dice.
	 */
	if(dice < 1)
		dice = 2;
	if(dice > 15)
		dice = 15;

	/*
	 * These next four lines make sure our dice have a (relatively)
	 * sane number of sides each.
	 */
	if(sides < 2)
		sides = 2;
	if(sides > 999)
		sides = 999;
	
	/*
	 * These next four lines make sure our bonus/penalty is sane.
	 */
	if(bonus < -999)
		bonus = -999;
	if(bonus > 999)
		bonus = 999;

	str = g_string_new("");	/* Allocate ourselves an empty string for output. */
	if(i)	/* Show the output in dice notation format. */
	{
		g_string_append_printf(str, "%dd%d", dice, sides);	/* For example, 1d20 */
		if(bonus > 0)
			g_string_append_printf(str, "+%d", bonus);	/* 1d20+5 */
		else if(bonus < 0)
			g_string_append_printf(str, "%d", bonus);	/* 1d20-3 (saying "-%d" would be redundant, since the '-' gets output with bonus automatically) */
		g_string_append_printf(str, ":");	/* Final colon.  1d20-4: */
	}
	else	/* Show the output in legacy format. */
	{
		g_string_append_printf(str, "Rolls %d %d-sided %s:", dice, sides,
								(dice == 1) ? "die" : "dice");	/* If we have one die use the singular, else the plural (dice). */
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

	if(conv->type == PURPLE_CONV_TYPE_IM)	/* If we're in a one-on-one IM */
		purple_conv_im_send(PURPLE_CONV_IM(conv), str->str);	/* Send our output as an IM */
	else if(conv->type == PURPLE_CONV_TYPE_CHAT)	/* If we're in a chat room */
		purple_conv_chat_send(PURPLE_CONV_CHAT(conv), str->str);	/* Send our output to the channel */

	g_string_free(str, TRUE);	/* Free our output string, as we're done with it now. */

	return PURPLE_CMD_RET_OK;	/* Never give up!  Never surrender! */
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	const gchar *help;

	help = _("dice [dice] [sides]:  rolls dice number of sides sided dice OR\ndice XdY+-Z:  rolls X number of Y sided dice, giving a Z bonus/penalty to each.  e.g. 1d20+2");

	dice_cmd_id = purple_cmd_register("dice", "wws", PURPLE_CMD_P_PLUGIN,
									PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT |
									PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS,
									NULL, PURPLE_CMD_FUNC(roll),
									help, NULL);

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
	PURPLE_PLUGIN_MAGIC,								/**< magic			*/
	PURPLE_MAJOR_VERSION,								/**< major version	*/
	PURPLE_MINOR_VERSION,								/**< minor version	*/
	PURPLE_PLUGIN_STANDARD,								/**< type			*/
	NULL,												/**< ui_requirement	*/
	0,													/**< flags			*/
	NULL,												/**< dependencies	*/
	PURPLE_PRIORITY_DEFAULT,							/**< priority		*/

	"core-plugin_pack-dice",							/**< id				*/
	NULL,												/**< name			*/
	PP_VERSION,											/**< version		*/
	NULL,												/**  summary		*/
	NULL,												/**  description	*/
	"Gary Kramlich <grim@reaperworld.com>",				/**< author			*/
	PP_WEBSITE,											/**< homepage		*/

	plugin_load,										/**< load			*/
	plugin_unload,										/**< unload			*/
	NULL,												/**< destroy		*/

	NULL,												/**< ui_info		*/
	NULL,												/**< extra_info		*/
	NULL,												/**< prefs_info		*/
	NULL,												/**< actions		*/
	NULL,												/**< reserved 1		*/
	NULL,												/**< reserved 2		*/
	NULL,												/**< reserved 3		*/
	NULL												/**< reserved 4		*/
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

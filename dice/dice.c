/*
 * Adds a command to roll an arbitrary number of dice with an arbitrary 
 * number of sides
 * Copyright (C) 2005 Gary Kramlich <grim@reaperworld.com>
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
# include "../pp_config.h"
#endif /* HAVE_CONFIG_H */

#define PURPLE_PLUGINS

#include <glib.h>
#include <time.h>
#include <stdlib.h>

#include <cmds.h>
#include <conversation.h>
#include <debug.h>
#include <plugin.h>
#include <version.h>

#include "../common/pp_internal.h"

static PurpleCmdId dice_cmd_id = 0;

static PurpleCmdRet
roll(PurpleConversation *conv, const gchar *cmd, gchar **args,
	 gchar *error, void *data)
{
	GString *str = NULL;
	gint dice = 2, sides = 6, roll, i;

	srand(time(NULL));

	if(args[0])
		dice = atoi(args[0]);

	if(args[1])
		sides = atoi(args[1]);

	if(dice < 1)
		dice = 2;
	if(dice > 15)
		dice = 15;

	if(sides < 2)
		sides = 2;
	if(sides > 999)
		sides = 999;

	str = g_string_new("");
	g_string_append_printf(str, "Rolls %d %d-sided %s:", dice, sides,
						   (dice == 1) ? "die" : "dice");

	for(i = 0; i < dice; i++) {
		roll = rand() % sides + 1;
		g_string_append_printf(str, " %d", roll);
	}

	if(conv->type == PURPLE_CONV_TYPE_IM)
		purple_conv_im_send(PURPLE_CONV_IM(conv), str->str);
	else if(conv->type == PURPLE_CONV_TYPE_CHAT)
		purple_conv_chat_send(PURPLE_CONV_CHAT(conv), str->str);

	g_string_free(str, TRUE);

	return PURPLE_CMD_RET_OK;
}

static gboolean
plugin_load(PurplePlugin *plugin) {
	const gchar *help;

	help = _("dice [dice] [sides]:  rolls dice number of sides sided dice");

	dice_cmd_id = purple_cmd_register("dice", "wws", PURPLE_CMD_P_PLUGIN,
									PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT |
									PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS,
									NULL, PURPLE_CMD_FUNC(roll),
									help, NULL);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin) {
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
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Dice");
	info.summary = _("Rolls dice in a chat or im");
	info.description = _("Adds a command (/dice) to roll an arbitrary "
						 "number of dice with an arbitrary number of sides");

}

PURPLE_INIT_PLUGIN(flip, init_plugin, info)

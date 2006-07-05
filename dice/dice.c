/*
 * Adds a command to roll an arbitrary number of dice with an arbitrary 
 * number of sides
 * Copyright (C) 2005 Gary Kramlich <amc_grim@users.sf.net>
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
#if HAVE_CONFIG_H
# include "../gpp_config.h"
#endif /* HAVE_CONFIG_H */

#define GAIM_PLUGINS

#include <glib.h>
#include <time.h>
#include <stdlib.h>

#include <cmds.h>
#include <conversation.h>
#include <debug.h>
#include <plugin.h>
#include <version.h>

#include "../common/i18n.h"

static GaimCmdId dice_cmd_id = 0;

static GaimCmdRet
roll(GaimConversation *conv, const gchar *cmd, gchar **args,
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

	if(conv->type == GAIM_CONV_TYPE_IM)
		gaim_conv_im_send(GAIM_CONV_IM(conv), str->str);
	else if(conv->type == GAIM_CONV_TYPE_CHAT)
		gaim_conv_chat_send(GAIM_CONV_CHAT(conv), str->str);

	g_string_free(str, TRUE);

	return GAIM_CMD_RET_OK;
}

static gboolean
plugin_load(GaimPlugin *plugin) {
	const gchar *help;

	help = _("dice [dice] [sides]:  rolls dice number of sides sided dice");

	dice_cmd_id = gaim_cmd_register("dice", "wws", GAIM_CMD_P_PLUGIN,
									GAIM_CMD_FLAG_IM | GAIM_CMD_FLAG_CHAT |
									GAIM_CMD_FLAG_ALLOW_WRONG_ARGS,
									NULL, GAIM_CMD_FUNC(roll),
									help, NULL);

	return TRUE;
}

static gboolean
plugin_unload(GaimPlugin *plugin) {
	gaim_cmd_unregister(dice_cmd_id);

	return TRUE;
}

static GaimPluginInfo info =
{
	GAIM_PLUGIN_MAGIC,								/**< magic			*/
	GAIM_MAJOR_VERSION,								/**< major version	*/
	GAIM_MINOR_VERSION,								/**< minor version	*/
	GAIM_PLUGIN_STANDARD,							/**< type			*/
	NULL,											/**< ui_requirement	*/
	0,												/**< flags			*/
	NULL,											/**< dependencies	*/
	GAIM_PRIORITY_DEFAULT,							/**< priority		*/

	"core-plugin_pack-dice",						/**< id				*/
	NULL,											/**< name			*/
	GPP_VERSION,									/**< version		*/
	NULL,											/**  summary		*/
	NULL,											/**  description	*/
	"Gary Kramlich <amc_grim@users.sf.net>",		/**< author			*/
	GPP_WEBSITE,									/**< homepage		*/

	plugin_load,									/**< load			*/
	plugin_unload,									/**< unload			*/
	NULL,											/**< destroy		*/

	NULL,											/**< ui_info		*/
	NULL,											/**< extra_info		*/
	NULL,											/**< prefs_info		*/
	NULL											/**< actions		*/
};

static void
init_plugin(GaimPlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GPP_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GPP_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Dice");
	info.summary = _("Rolls dice in a chat or im");
	info.description = _("Adds a command (/dice) to roll an arbitrary "
						 "number of dice with an arbitrary number of sides");

}

GAIM_INIT_PLUGIN(flip, init_plugin, info)

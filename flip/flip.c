/*
 * Adds a command to flip a coin in a conversation and outputs the result
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

#include "../common/i18n.h"

static PurpleCmdId flip_cmd_id = 0;

static PurpleCmdRet
flip_it(PurpleConversation *conv, const gchar *cmd, gchar **args,
		gchar *error, void *data)
{
	gboolean heads = FALSE;
	gchar *msg;

	srand(time(NULL));

	heads = rand() % 2;

	msg = g_strdup_printf("Flips a coin: %s", (heads) ? "HEADS" : "TAILS");

	if(conv->type == PURPLE_CONV_TYPE_IM)
		purple_conv_im_send(PURPLE_CONV_IM(conv), msg);
	else if(conv->type == PURPLE_CONV_TYPE_CHAT)
		purple_conv_chat_send(PURPLE_CONV_CHAT(conv), msg);

	g_free(msg);

	return PURPLE_CMD_RET_OK;
}

static gboolean
plugin_load(PurplePlugin *plugin) {
	flip_cmd_id = purple_cmd_register("flip", "", PURPLE_CMD_P_PLUGIN,
									PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT,
									NULL, PURPLE_CMD_FUNC(flip_it),
									_("Outputs the results of flipping a coin"),
									NULL);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin) {
	purple_cmd_unregister(flip_cmd_id);

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

	"core-plugin_pack-flip",							/**< id				*/
	NULL,												/**< name			*/
	PP_VERSION,											/**< version		*/
	NULL,												/**  summary		*/
	NULL,												/**  description	*/
	"Gary Kramlich <amc_grim@users.sf.net>",			/**< author			*/
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
	NULL,												/**< reserved 4		*/
};

static void
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Coin Flip");
	info.summary = _("Flips a coin and outputs the result");
	info.description = _("Adds a command (/flip) to flip a coin and "
						 "outputs the result in the active conversation");
}

PURPLE_INIT_PLUGIN(flip, init_plugin, info)

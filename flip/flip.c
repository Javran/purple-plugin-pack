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

static GaimCmdId flip_cmd_id = 0;

static GaimCmdRet
flip_it(GaimConversation *conv, const gchar *cmd, gchar **args,
		gchar *error, void *data)
{
	gboolean heads = FALSE;
	gchar *msg;

	srand(time(NULL));

	heads = rand() % 2;

	msg = g_strdup_printf("Flips a coin: %s", (heads) ? "HEADS" : "TAILS");

	if(conv->type == GAIM_CONV_TYPE_IM)
		gaim_conv_im_send(GAIM_CONV_IM(conv), msg);
	else if(conv->type == GAIM_CONV_TYPE_CHAT)
		gaim_conv_chat_send(GAIM_CONV_CHAT(conv), msg);

	g_free(msg);

	return GAIM_CMD_RET_OK;
}

static gboolean
plugin_load(GaimPlugin *plugin) {
	flip_cmd_id = gaim_cmd_register("flip", "", GAIM_CMD_P_PLUGIN,
									GAIM_CMD_FLAG_IM | GAIM_CMD_FLAG_CHAT,
									NULL, GAIM_CMD_FUNC(flip_it),
									_("Outputs the results of flipping a coin"),
									NULL);

	return TRUE;
}

static gboolean
plugin_unload(GaimPlugin *plugin) {
	gaim_cmd_unregister(flip_cmd_id);

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

	"core-plugin_pack-flip",						/**< id				*/
	NULL,											/**< name			*/
	GPP_VERSION,									/**< version		*/
	NULL,											/**  summary		*/
	NULL,											/**  description	*/
	"Gary Kramlich <amc_grim@users.sf.net>",		/**< author			*/
	NULL,											/**< homepage		*/

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
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Coin Flip");
	info.summary = _("Flips a coin and outputs the result");
	info.description = _("Adds a command (/flip) to flip a coin and "
						 "outputs the result in the active conversation");
}

GAIM_INIT_PLUGIN(flip, init_plugin, info)

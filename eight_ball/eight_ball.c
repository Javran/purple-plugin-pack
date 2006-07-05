/*
 * Gaim Plugin Pack
 * Copyright (C) 2003-2005
 * See ../AUTHORS for a list of all authors
 *
 * eight_ball: Provides Magic 8-ball-like functionality
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
#	include "../gpp_config.h"
#endif

#include "../common/i18n.h"

/* libc */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* GLib */
#include <glib.h>

/* Gaim */
#define GAIM_PLUGINS
#include <cmds.h>
#include <conversation.h>
#include <debug.h>
#include <plugin.h>
#include <version.h>

/* TODO: add to these, but be careful NOT to use real 8-ball messages!!! */
static const gchar *eight_ball_strings[] = {
	"Unclear",
	"Certainly",
	"Ask again later",
	"Not likely",
	"It's possible",
};

/* feel free to add */
static const gchar *sg_ball_strings[] = {
	"Indeed",
	"I got nothin\'",
	"Ask me tomorrow", /* from "Window of Opportunity" */
	"According to quantum physics, it's theoretically possible...",
	"That's what you get for dickin' around", /* from "Fallout" */
	"Jaffa! Cree!",
	"Tec ma te",
	"Kneel before your god!"
};

static GaimCmdId eight_ball_cmd_id = 0, sg_ball_cmd_id = 0;

static GaimCmdRet
eight_ball_cmd_func(GaimConversation *conv, const gchar *cmd, gchar **args,
		gchar *error, void *data)
{
	GString *msgstr = NULL;
	gchar *msgprefix = NULL;
	const gchar **msgs = NULL;
	gint numstrings = 0, index = 0;

	msgstr = g_string_new("");

	srand(time(NULL));

	if(!strcmp(cmd, "sgball")) {
		numstrings = sizeof(sg_ball_strings) / sizeof(sg_ball_strings[0]);
		msgprefix = "The Gaim Stargate Ball says";
		msgs = sg_ball_strings;
	} else {
		numstrings = sizeof(eight_ball_strings) / sizeof(eight_ball_strings[0]);
		msgprefix = "The Gaim 8 Ball says";
		msgs = eight_ball_strings;
	}

	if(*args == NULL || args[0] == NULL)
		index = rand() % numstrings;
	else
		index = atoi(args[0]);

	if(index < 0)
		index *= -1;
	
	if(index > numstrings)
		index %= numstrings;

	g_string_append_printf(msgstr, "%s:  %s", msgprefix, msgs[index]);

	switch(gaim_conversation_get_type(conv)) {
		case GAIM_CONV_TYPE_IM:
			gaim_conv_im_send(GAIM_CONV_IM(conv), msgstr->str);
			break;
		case GAIM_CONV_TYPE_CHAT:
			gaim_conv_chat_send(GAIM_CONV_CHAT(conv), msgstr->str);
			break;
		default:
			g_string_free(msgstr, TRUE);
			return GAIM_CMD_RET_FAILED;
	}

	g_string_free(msgstr, TRUE);

	return GAIM_CMD_RET_OK;
}

static gboolean
plugin_load(GaimPlugin *plugin)
{
	const gchar *eight_ball_help, *sg_ball_help;

	eight_ball_help = _("8ball:  sends a random 8ball message");
	sg_ball_help = _("sgball:  sends a random Stargate Ball message");

	eight_ball_cmd_id = gaim_cmd_register("8ball", "w", GAIM_CMD_P_PLUGIN,
									GAIM_CMD_FLAG_IM | GAIM_CMD_FLAG_CHAT |
									GAIM_CMD_FLAG_ALLOW_WRONG_ARGS,	NULL,
									GAIM_CMD_FUNC(eight_ball_cmd_func),
									eight_ball_help, NULL);

	sg_ball_cmd_id = gaim_cmd_register("sgball", "w", GAIM_CMD_P_PLUGIN,
									GAIM_CMD_FLAG_IM | GAIM_CMD_FLAG_CHAT |
									GAIM_CMD_FLAG_ALLOW_WRONG_ARGS, NULL,
									GAIM_CMD_FUNC(eight_ball_cmd_func),
									sg_ball_help, NULL);

	return TRUE;
}

static gboolean
plugin_unload(GaimPlugin *plugin)
{
	gaim_cmd_unregister(eight_ball_cmd_id);
	gaim_cmd_unregister(sg_ball_cmd_id);

	return TRUE;
}

static GaimPluginInfo eight_ball_info =
{
	GAIM_PLUGIN_MAGIC, /* Do you believe in magic? */
	GAIM_MAJOR_VERSION,
	GAIM_MINOR_VERSION,
	GAIM_PLUGIN_STANDARD,
	NULL,
	0,
	NULL,
	GAIM_PRIORITY_DEFAULT,

	"core-plugin_pack-eight_ball",
	NULL,
	GPP_VERSION,
	NULL,
	NULL,
	"John Bailey <rekkanoryo@users.sourceforge.net>",
	GPP_WEBSITE,

	plugin_load,
	plugin_unload,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(GaimPlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GPP_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GPP_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	eight_ball_info.name = _("Magic 8 Ball");
	eight_ball_info.summary = _("Provides Magic 8-ball like functionality");
	eight_ball_info.description = _("Provides Magic 8-ball like functionality "
								"with the /8ball command, as well as similar "
								"functionality for common Stargate words or "
								"phrases with the /sg-ball command.");

	return;
}

GAIM_INIT_PLUGIN(eight_ball, init_plugin, eight_ball_info)


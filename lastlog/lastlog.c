/*
 * LastLog - Searches the conversation log for a word or phrase
 * Copyright (C) 2006 John Bailey <rekkanoryo@rekkanoryo.org>
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
#endif

/* this has to be defined for the plugin to work correctly */
#define GAIM_PLUGINS

/* Gaim headers */
#include <cmds.h>
#include <conversation.h>
#include <debug.h>
#include <plugin.h>
#include <util.h>
#include <version.h>

static GaimCmdId lastlog_cmd;

static gboolean
lastlog_load(GaimPlugin *plugin)
{
	const gchar *help;

	help = _("lastlog &lt;text&gt;, shows messages in the current "
			"conversation's history that contain the text specified in "
			"&lt;text&gt;.\n");

	lastlog_cmd = gaim_cmd_register

static GaimPluginInfo lastlog_info =
{
	GAIM_PLUGIN_MAGIC,		/* abracadabra */
	GAIM_MAJOR_VERSION,							
	GAIM_MINOR_VERSION,
	GAIM_PLUGIN_STANDARD,						
	NULL,
	0,
	NULL,
	GAIM_PRIORITY_DEFAULT,
	"core-plugin_pack-lastlog",
	NULL,
	GPP_VERSION,
	NULL,
	NULL,
	"John Bailey <rekkanoryo@rekkanoryo.org",
	GPP_WEBSITE,
	lastlog_load,
	lastlog_unload,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static void
lastlog_init(GaimPlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GPP_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GPP_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	lastlog_info.name = _("Lastlog");
	lastlog_info.summary = _("A history searching plugin");
	lastlog_info.description =
		_("Adds the /lastlog command to allow searching through the history in"
			"a conversation, much like the IRC client irssi has.");
}

GAIM_INIT_PLUGIN(lastlog, lastlog_init, lastlog_info)


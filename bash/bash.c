/*
 * Gaim Plugin Pack
 * Copyright (C) 2003-2005
 * See ../AUTHORS for a list of all authors
 *
 * bash: provides a /command to display the URL for a random or specified
 * 		bash.org quote.
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

#define BASH_QUOTES 636661
#define QDB_QUOTES 58841

static GaimCmdId bash, qdb;

static GaimCmdRet
cmd_func(GaimConversation *conv, const gchar *cmd, gchar **args,
		gchar *error, void *data)
{
	GString *msgstr = NULL;
	guint32 quotes = 0, quoteid = 0;
	
	msgstr = g_string_new("");

	srand(time(NULL));

	if(!strcmp(cmd, "bash")) {
		g_string_append(msgstr, "http://www.bash.org/?");
		quotes = BASH_QUOTES;
	} else {
		g_string_append(msgstr, "http://qdb.us/");
		quotes = QDB_QUOTES;
	}

	if(*args == NULL || args[0] == NULL)
		quoteid = (rand() % quotes) + 1;
	else
		quoteid = atoi(args[0]);

	if(quoteid > quotes)
		quoteid %= quotes;
	
	g_string_append_printf(msgstr, "%i", quoteid);

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
	const gchar *bash_help, *qdb_help;
	GaimCmdFlag flags = GAIM_CMD_FLAG_IM | GAIM_CMD_FLAG_CHAT |
						GAIM_CMD_FLAG_ALLOW_WRONG_ARGS;

	bash_help = _("bash [n]: sends a link to a bash.org quote.  Specify a"
				" number for n and it will send a link to the quote with the"
				" specified number.");

	qdb_help = _("qdb [n]: sends a link to a qdb.us quote.  Specify a number "
				"for n and it will send a link to the quite with the specified "
				"number.");

	bash = gaim_cmd_register("bash", "w", GAIM_CMD_P_PLUGIN, flags, NULL,
							GAIM_CMD_FUNC(cmd_func), bash_help, NULL);

	qdb = gaim_cmd_register("qdb", "w", GAIM_CMD_P_PLUGIN, flags, NULL,
							GAIM_CMD_FUNC(cmd_func), qdb_help, NULL);

	return TRUE;
}

static gboolean
plugin_unload(GaimPlugin *plugin)
{
	gaim_cmd_unregister(bash);
	gaim_cmd_unregister(qdb);

	return TRUE;
}

static GaimPluginInfo bash_info =
{
	GAIM_PLUGIN_MAGIC, /* magic, my ass */
	GAIM_MAJOR_VERSION,
	GAIM_MINOR_VERSION,
	GAIM_PLUGIN_STANDARD,
	NULL,
	0,
	NULL,
	GAIM_PRIORITY_DEFAULT,
	"core-plugin_pack-bash",
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
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	bash_info.name = _("bash.org");
	bash_info.summary =
					_("Generates links for quotes at bash.org");
	bash_info.description =
					_("Generates links for quotes at bash.org or allows the "
					"user to specify a quote.  Provides the /bash command.");

	return;
}

GAIM_INIT_PLUGIN(bash, init_plugin, bash_info)

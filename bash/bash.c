/*
 * Purple Plugin Pack
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301, USA.
 */

/* If you can't figure out what this line is for, DON'T TOUCH IT. */
#include "../common/pp_internal.h"

/* libc */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Purple */
#include <cmds.h>
#include <conversation.h>
#include <debug.h>
#include <plugin.h>

#define BASH_QUOTES 636661
#define QDB_QUOTES 58841

static PurpleCmdId bash, qdb;

static PurpleCmdRet
cmd_func(PurpleConversation *conv, const gchar *cmd, gchar **args,
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

	switch(purple_conversation_get_type(conv)) {
		case PURPLE_CONV_TYPE_IM:
			purple_conv_im_send(PURPLE_CONV_IM(conv), msgstr->str);
			break;
		case PURPLE_CONV_TYPE_CHAT:
			purple_conv_chat_send(PURPLE_CONV_CHAT(conv), msgstr->str);
			break;
		default:
			g_string_free(msgstr, TRUE);
			return PURPLE_CMD_RET_FAILED;
	}

	g_string_free(msgstr, TRUE);

	return PURPLE_CMD_RET_OK;
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	const gchar *bash_help, *qdb_help;
	PurpleCmdFlag flags = PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT |
						PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS;

	bash_help = _("bash [n]: sends a link to a bash.org quote.  Specify a"
				" number for n and it will send a link to the quote with the"
				" specified number.");

	qdb_help = _("qdb [n]: sends a link to a qdb.us quote.  Specify a number "
				"for n and it will send a link to the quite with the specified "
				"number.");

	bash = purple_cmd_register("bash", "w", PURPLE_CMD_P_PLUGIN, flags, NULL,
							PURPLE_CMD_FUNC(cmd_func), bash_help, NULL);

	qdb = purple_cmd_register("qdb", "w", PURPLE_CMD_P_PLUGIN, flags, NULL,
							PURPLE_CMD_FUNC(cmd_func), qdb_help, NULL);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	purple_cmd_unregister(bash);
	purple_cmd_unregister(qdb);

	return TRUE;
}

static PurplePluginInfo bash_info =
{
	PURPLE_PLUGIN_MAGIC, /* magic, my ass */
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,
	"core-plugin_pack-bash",
	NULL,
	PP_VERSION,
	NULL,
	NULL,
	"John Bailey <rekkanoryo@rekkanoryo.org>",
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

	bash_info.name = _("bash.org");
	bash_info.summary =
					_("Generates links for quotes at bash.org");
	bash_info.description =
					_("Generates links for quotes at bash.org or allows the "
					"user to specify a quote.  Provides the /bash command.");

	return;
}

PURPLE_INIT_PLUGIN(bash, init_plugin, bash_info)

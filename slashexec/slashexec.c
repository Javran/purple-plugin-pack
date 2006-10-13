/*
 * slashexec - A CLI for gaim
 * Copyright (C) 2004-2006 Gary Kramlich
 * Copyright (C) 2005-2006 Peter Lawler
 * Copyright (C) 2005-2006 Daniel Atallah
 * Copyright (C) 2005-2006 John Bailey
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
#if defined HAVE_CONFIG_H && !defined _WIN32
# include "../gpp_config.h"
#endif

#include "../common/i18n.h"

#include <glib.h>
#include <stdarg.h>
#include <string.h>

#ifndef _WIN32
# include <pwd.h>
# include <unistd.h>
#endif

#define GAIM_PLUGINS

#include <cmds.h>
#include <conversation.h>
#include <debug.h>
#include <notify.h>
#include <plugin.h>
#include <util.h>
#include <version.h>

#ifdef _WIN32
/* Windows 2000 and earlier only allow 2047 bytes in an argv vector for cmd.exe
 * so we need to make sure we don't exceed that. 2036 allows "cmd.exe /c " to
 * fit inside the vector. */
# define MAX_CMD_LEN 2036
#else
/* Some unixes allow only 8192 chars in an argv vector; allow some space for
 * the shell command itself to fit in the vector.  Besides, if anyone is
 * crazy enough to use /exec for a command this long they need shot to 
 * begin with, or educated on the beauty of shell scripts. */
# define MAX_CMD_LEN 8000
#endif

static GaimCmdId se_cmd;
static GString *shell;

static void /* replace a character at the end of a string with' \0' */
se_replace_ending_char(gchar *string, gchar replace)
{
	gint stringlen = strlen(string);
	gchar *replace_in_string = g_utf8_strrchr(string, -1, replace);

	while(replace_in_string && replace_in_string == &(string[stringlen - 1])) {
		gaim_debug_info("slashexec", "Replacing %c at position %d\n",
				replace, stringlen - 1);
		*replace_in_string = '\0';
		stringlen = strlen(string);
		replace_in_string = g_utf8_strrchr(string, -1, replace);
	}

	return;
}

static gboolean
do_action(GaimConversation *conv, gchar *args, gboolean send)
{
	GError *parse_error = NULL, *exec_error = NULL;
	GString *command = NULL;
	gchar *spawn_cmd = NULL, **cmd_argv = NULL, *cmd_stdout = NULL,
		  *cmd_stderr = NULL;
#ifdef _WIN32
	const gchar *shell_flag = "/c";
#else
	const gchar *shell_flag = "-c";
#endif
	gint cmd_argc = 0;

	command = g_string_new("");

	/* we can still end up with 0 "args", and we need to make sure we don't
	 * go over the max command length */
	if(strlen(args) == 0 || strlen(args) > MAX_CMD_LEN)
		return FALSE;

	/* remove trailing \ characters to prevent some nasty crashes */
	se_replace_ending_char(args, '\\');

#ifndef _WIN32
	/* escape remaining special characters; this is to ensure the shell gets
	 * what the user actually typed. */
	args = g_strescape(args, "");
#endif

	/* if we get this far with a NULL, there's a problem. */
	if(!args) {
		gaim_debug_info("slashexec", "bad args: %s\n",
				args ? args : "null");
		
		return FALSE;
	}

	/* make sure the string passes the UTF8 validation in glib */
	if(!g_utf8_validate(args, -1, NULL)) {
		gaim_debug_info("slashexec", "invalid UTF8: %s\n",
				args ? args : "null");

		return FALSE;
	}

#ifdef _WIN32
	g_string_append_printf(command, "%s %s %s", shell->str, shell_flag,
			args);
#else
	g_string_append_printf(command, "%s %s \"%s\"", shell->str, shell_flag,
			args);
#endif

	/* This is the finished command that will be executed */
	spawn_cmd = command->str;

	/* We need the command parsed into a proper argv for it to be executed */
	if(!g_shell_parse_argv(spawn_cmd, &cmd_argc, &cmd_argv, &parse_error)) {
		/* everything in here is error checking and information for the user */

		/* the command string isn't NULL, so give it to the user */
		if(spawn_cmd) {
			GString *errmsg = g_string_new("");
			g_string_append_printf(errmsg, _("Unable to parse \"%s\""),
					spawn_cmd);
			gaim_debug_info("slashexec", "%s\n", errmsg->str);
			gaim_conversation_write(conv, NULL, errmsg->str,
					GAIM_MESSAGE_SYSTEM, time(NULL));
			g_string_free(errmsg, TRUE);
		}

		/* the GError isn't NULL, so give its information to the user */
		if(parse_error) {
			GString *errmsg = g_string_new("");
			g_string_append_printf(errmsg, _("Parse error message: %s"),
					parse_error->message ? parse_error->message : "null");
			gaim_debug_info("slashexec", "%s\n", errmsg->str);
			gaim_conversation_write(conv, NULL, errmsg->str,
					GAIM_MESSAGE_SYSTEM, time(NULL));
			g_error_free(parse_error);
			g_string_free(errmsg, TRUE);
		}

		if(cmd_argv)
			g_strfreev(cmd_argv);

		return FALSE;
	}

	/* Now we're ready to execute the user's command.  Let the user know. */
	gaim_conversation_write(conv, NULL,
			_("Executing your command.  This will cause a pause until the "
			"command finishes executing.  This notice has not been sent."),
			GAIM_MESSAGE_SYSTEM, time(NULL));

	/* I may eventually add a pref to show this to the user; for now it's fine
	 * going just to the debug bucket */
	gaim_debug_info("slashexec", "Spawn command: %s\n", spawn_cmd);

	/* now we actually execute the command.  everything inside the block
	 * is error checking and information for the user. */
	if(!g_spawn_sync(NULL, cmd_argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
				&cmd_stdout, &cmd_stderr, NULL, &exec_error))
	{
		/* the command isn't NULL, so let the user see it */
		if(spawn_cmd) {
			GString *errmsg = g_string_new("");
			g_string_append_printf(errmsg, _("Unable to execute \"%s\""),
					spawn_cmd);
			gaim_debug_info("slashexec", "%s\n", errmsg->str);
			gaim_conversation_write(conv, NULL, errmsg->str,
					GAIM_MESSAGE_SYSTEM, time(NULL));
			g_string_free(errmsg, TRUE);
		}

		/* the GError isn't NULL, so let's show the user its information */
		if(exec_error) {
			GString *errmsg = g_string_new("");
			g_string_append_printf(errmsg, _("Execute error message: %s"),
					exec_error->message ? exec_error->message : "null");
			gaim_debug_info("slashexec", "%s\n", errmsg->str);
			gaim_conversation_write(conv, NULL, errmsg->str,
					GAIM_MESSAGE_SYSTEM, time(NULL));
			g_error_free(exec_error);
			g_string_free(errmsg, TRUE);
		}

		g_free(cmd_stdout);
		g_free(cmd_stderr);
		g_strfreev(cmd_argv);
		
		return FALSE;
	}

	/* if we get this far then the command actually executed */
	if(parse_error)
		g_error_free(parse_error);

	if(exec_error)
		g_error_free(exec_error);

	if(cmd_stderr)
		gaim_debug_info("slashexec", "command stderr: %s\n", cmd_stderr);

	g_strfreev(cmd_argv);
	g_free(cmd_stderr);

	if(cmd_stdout && strcmp(cmd_stdout, "") && strcmp(cmd_stdout, "\n")) {
		g_strchug(cmd_stdout);

		if(g_str_has_suffix(cmd_stdout, "\n"))
			cmd_stdout[strlen(cmd_stdout) - 1] = '\0';

		if(send) {
			GaimConversationType type = gaim_conversation_get_type(conv);
			
			gaim_debug_info("slashexec", "Command stdout: %s\n", cmd_stdout);

			switch(type) {
				case GAIM_CONV_TYPE_IM:
					gaim_conv_im_send(GAIM_CONV_IM(conv), cmd_stdout);
					break;
				case GAIM_CONV_TYPE_CHAT:
					gaim_conv_chat_send(GAIM_CONV_CHAT(conv), cmd_stdout);
					break;
				default:
					return FALSE;
			}
		} else
			gaim_conversation_write(conv, NULL, cmd_stdout, GAIM_MESSAGE_SYSTEM,
					time(NULL));
	} else {
		gaim_debug_info("slashexec", "Error executing \"%s\"\n", spawn_cmd);
		gaim_conversation_write(conv, NULL,
				_("There was an error executing your command."),
				GAIM_MESSAGE_SYSTEM, time(NULL));
			
		g_free(spawn_cmd);
		g_free(cmd_stdout);

		return FALSE;
	}
	
	g_free(cmd_stdout);
	g_free(spawn_cmd);

	return TRUE;
}

static GaimCmdRet
se_cmd_cb(GaimConversation *conv, const gchar *cmd, gchar **args, gchar **error,
			gpointer data)
{
	gboolean send = FALSE;
	char *string = args[0];
	if(string && !strncmp(string, "-o", 2)) {
		send = TRUE;
		string += 3;
	}

	if (do_action(conv, string, send))
		return GAIM_CMD_RET_OK;
	else
		return GAIM_CMD_RET_FAILED;
}

static void
se_sending_msg_helper(GaimConversation *conv, char **message)
{
	/* 'recurse' is used to detect a recursion. If the user sends "!!!command",
	 * then it is changed to "!command" and sent to the user. We do not want to
	 * process that in such cases. */
	static gboolean recurse = FALSE;
	char *string = *message, *strip;
	gboolean send = TRUE;

	if (recurse)
		return;
	if (conv == NULL)
		return;

	strip = gaim_markup_strip_html(string);
	if (*strip != '!') {
		g_free(strip);
		return;
	}
	*message = NULL;
	g_free(string);

	if (strncmp(strip, "!!!", 3) == 0) {
		recurse = TRUE;
		switch(gaim_conversation_get_type(conv)) {
			case GAIM_CONV_TYPE_IM:
				gaim_conv_im_send(GAIM_CONV_IM(conv), strip + 2);
				break;
			case GAIM_CONV_TYPE_CHAT:
				gaim_conv_chat_send(GAIM_CONV_CHAT(conv), strip + 2);
				break;
			default:
				break;
		}
		g_free(strip);
		recurse = FALSE;
		return;
	}

	if (*(strip + 1) == '!')
		send = FALSE;

	if (send)
		do_action(conv, strip + 1, send);
	else
		do_action(conv, strip + 2, send);
	g_free(strip);
}

static void
se_sending_chat_msg_cb(GaimAccount *account, char **message, int id)
{
	GaimConversation *conv;
	conv = gaim_find_chat(account->gc, id);
	se_sending_msg_helper(conv, message);
}

static void
se_sending_im_msg_cb(GaimAccount *account, const char *who, char **message)
{
	GaimConversation *conv;
	conv = gaim_find_conversation_with_account(GAIM_CONV_TYPE_IM, who, account);
	se_sending_msg_helper(conv, message);
}

static gboolean
se_load(GaimPlugin *plugin) {
#ifndef _WIN32
	struct passwd *pw = NULL;
#endif /* _WIN32 */
	const gchar *help = _("exec [-o] &lt;command&gt;, runs the command.\n"
						"If the -o flag is used then output is sent to the"
						"current conversation; otherwise it is printed to the "
						"current text box.");
	se_cmd = gaim_cmd_register("exec", "s", GAIM_CMD_P_PLUGIN,
								GAIM_CMD_FLAG_IM | GAIM_CMD_FLAG_CHAT, NULL,
								se_cmd_cb, help, NULL);
	gaim_signal_connect(gaim_conversations_get_handle(), "sending-im-msg", plugin,
				GAIM_CALLBACK(se_sending_im_msg_cb), NULL);
	gaim_signal_connect(gaim_conversations_get_handle(), "sending-chat-msg", plugin,
				GAIM_CALLBACK(se_sending_chat_msg_cb), NULL);

#ifdef _WIN32
	shell = g_string_new("cmd.exe");
#else
	pw = getpwuid(getuid());
	
	if(pw)
		if(pw->pw_shell)
			shell = g_string_new(pw->pw_shell);
		else
			shell = g_string_new("/bin/sh");
	else
		shell = g_string_new("/bin/sh");
#endif /* _WIN32 */

	return TRUE;
}

static gboolean
se_unload(GaimPlugin *plugin) {
	gaim_cmd_unregister(se_cmd);

	g_string_free(shell, TRUE);

	return TRUE;
}

static GaimPluginInfo se_info = {
	GAIM_PLUGIN_MAGIC,
	GAIM_MAJOR_VERSION,
	GAIM_MINOR_VERSION,
	GAIM_PLUGIN_STANDARD,								/* type				*/
	NULL,												/* ui requirement	*/
	0,													/* flags			*/
	NULL,												/* dependencies		*/
	GAIM_PRIORITY_DEFAULT,								/* priority			*/
	"core-plugin_pack-slashexec",						/* id				*/
	"/exec",											/* name				*/
	GPP_VERSION,										/* version			*/
	NULL,												/* summary			*/
	NULL,												/* description		*/
	"Gary Kramlich <amc_grim@users.sf.net>,"
	" Peter Lawler <bleeter from users.sf.net>,"
	" Daniel Atallah,"
	" Sadrul Habib Chowdhury <sadrul@users.sf.net>, and"
	" John Bailey <rekkanoryo@users.sf.net>",			/* authors			*/
	GPP_WEBSITE,										/* homepage			*/
	se_load,											/* load				*/
	se_unload,											/* unload			*/
	NULL,												/* destroy			*/
	NULL,												/* ui info			*/
	NULL,												/* extra info		*/
	NULL,												/* actions info		*/
	NULL
};

static void
init_plugin(GaimPlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */
	se_info.summary = _("/exec a la UNIX IRC CLI");
	se_info.description = _("A plugin that adds the /exec command line"
							" interpreter like most UNIX/Linux IRC"
							" clients have.\n");
}

GAIM_INIT_PLUGIN(slashexec, init_plugin, se_info)

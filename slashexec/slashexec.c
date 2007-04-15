/*
 * slashexec - A CLI for gaim
 * Copyright (C) 2004-2006 Gary Kramlich
 * Copyright (C) 2005-2006 Peter Lawler
 * Copyright (C) 2005-2006 Daniel Atallah
 * Copyright (C) 2005-2006 John Bailey
 * Copyright (C) 2006 Sadrul Habib Chowdhury
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

/* XXX: Translations of strings - Several strings are NOT marked as needing to
 * be translated.  This is because they go only to the debug window.  I only
 * care to translate messages users will see in the main interface.  Debug
 * window messages are not important. - rekkanoryo */

#if defined HAVE_CONFIG_H && !defined _WIN32
# include "../pp_config.h"
#endif

#include "../common/i18n.h"

#include <glib.h>
#include <stdarg.h>
#include <string.h>

#ifndef _WIN32
# include <pwd.h>
# include <unistd.h>
#endif

#define PURPLE_PLUGINS

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

static PurpleCmdId se_cmd;
static gchar *shell;

static void /* replace a character at the end of a string with' \0' */
se_replace_ending_char(gchar *string, gchar replace)
{
	gint stringlen;
	gchar *replace_in_string;

	stringlen = strlen(string);
	replace_in_string = g_utf8_strrchr(string, -1, replace);

	/* continue to replace the character at the end of the string until the
	 * current last character in the string is not the character that needs to
	 * be replaced. */
	while(replace_in_string && replace_in_string == &(string[stringlen - 1])) {
		purple_debug_info("slashexec", "Replacing %c at position %d\n",
				replace, stringlen - 1);

		/* a \0 is the logical end of the string, even if characters exist in
		 * the string after it.  Replacing the bad character with a \0 causes
		 * the string to technically be shortened by one character even though
		 * the allocated size of the string doesn't change. */
		*replace_in_string = '\0';

		/* update the string length due to the new \0 */
		stringlen = strlen(string);

		/* find, if one exists, another instance of the bad character */
		replace_in_string = g_utf8_strrchr(string, -1, replace);
	}

	return;
}

static gboolean
se_do_action(PurpleConversation *conv, gchar *args, gboolean send)
{
	GError *parse_error = NULL, *exec_error = NULL;
	gchar *spawn_cmd = NULL, **cmd_argv = NULL, *cmd_stdout = NULL,
		  *cmd_stderr = NULL;
	gint cmd_argc = 0;
#ifdef _WIN32
	const gchar *shell_flag = "/c";
#else
	const gchar *shell_flag = "-c";
#endif

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
		purple_debug_info("slashexec", "args NULL!\n");
		
		return FALSE;
	}

	/* make sure the string passes the UTF8 validation in glib */
	if(!g_utf8_validate(args, -1, NULL)) {
		purple_debug_info("slashexec", "invalid UTF8: %s\n",
				args ? args : "null");

		return FALSE;
	}

#ifdef _WIN32
	spawn_cmd = g_strdup_printf("%s %s %s", shell, shell_flag, args);
#else
	spawn_cmd = g_strdup_printf("%s %s \"%s\"", shell, shell_flag, args);
#endif

	/* We need the command parsed into a proper argv for it to be executed */
	if(!g_shell_parse_argv(spawn_cmd, &cmd_argc, &cmd_argv, &parse_error)) {
		/* everything in here is error checking and information for the user */

		char *errmsg = NULL;

		/* the command string isn't NULL, so give it to the user */
		if(spawn_cmd) {
			errmsg = g_strdup_printf(_("Unable to parse \"%s\""), spawn_cmd);

			purple_debug_info("slashexec", "%s\n", errmsg);
			purple_conversation_write(conv, NULL, errmsg, PURPLE_MESSAGE_SYSTEM,
					time(NULL));

			g_free(errmsg);
		}

		/* the GError isn't NULL, so give its information to the user */
		if(parse_error) {
			errmsg = g_strdup_printf(_("Parse error message: %s"),
					parse_error->message ? parse_error->message : "null");

			purple_debug_info("slashexec", "%s\n", errmsg);
			purple_conversation_write(conv, NULL, errmsg, PURPLE_MESSAGE_SYSTEM,
					time(NULL));

			g_free(errmsg);
			g_error_free(parse_error);
		}

		if(cmd_argv)
			g_strfreev(cmd_argv);

		return FALSE;
	}

	/* I may eventually add a pref to show this to the user; for now it's fine
	 * going just to the debug bucket */
	purple_debug_info("slashexec", "Spawn command: %s\n", spawn_cmd);

	/* now we actually execute the command.  everything inside the block
	 * is error checking and information for the user. */
	if(!g_spawn_sync(NULL, cmd_argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
				&cmd_stdout, &cmd_stderr, NULL, &exec_error))
	{
		char *errmsg = NULL;

		/* the command isn't NULL, so let the user see it */
		if(spawn_cmd) {
			errmsg = g_strdup_printf(_("Unable to execute \"%s\""),	spawn_cmd);

			purple_debug_info("slashexec", "%s\n", errmsg);
			purple_conversation_write(conv, NULL, errmsg, PURPLE_MESSAGE_SYSTEM,
					time(NULL));

			g_free(errmsg);
		}

		/* the GError isn't NULL, so let's show the user its information */
		if(exec_error) {
			errmsg = g_strdup_printf(_("Execute error message: %s"),
					exec_error->message ? exec_error->message : "NULL");

			purple_debug_info("slashexec", "%s\n", errmsg);
			purple_conversation_write(conv, NULL, errmsg,	PURPLE_MESSAGE_SYSTEM,
					time(NULL));

			g_free(errmsg);
			g_error_free(exec_error);
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
		purple_debug_info("slashexec", "command stderr: %s\n", cmd_stderr);

	g_strfreev(cmd_argv);
	g_free(cmd_stderr);

	if(cmd_stdout && strcmp(cmd_stdout, "") && strcmp(cmd_stdout, "\n")) {
		g_strchug(cmd_stdout);

		if(g_str_has_suffix(cmd_stdout, "\n"))
			cmd_stdout[strlen(cmd_stdout) - 1] = '\0';

		if(send) {
			purple_debug_info("slashexec", "Command stdout: %s\n", cmd_stdout);

			switch(purple_conversation_get_type(conv)) {
				case PURPLE_CONV_TYPE_IM:
					purple_conv_im_send(PURPLE_CONV_IM(conv), cmd_stdout);
					break;
				case PURPLE_CONV_TYPE_CHAT:
					purple_conv_chat_send(PURPLE_CONV_CHAT(conv), cmd_stdout);
					break;
				default:
					return FALSE;
			}

		} else
			purple_conversation_write(conv, NULL, cmd_stdout, PURPLE_MESSAGE_SYSTEM,
					time(NULL));
	} else {
		purple_debug_info("slashexec", "Error executing \"%s\"\n", spawn_cmd);
		purple_conversation_write(conv, NULL,
				_("There was an error executing your command."),
				PURPLE_MESSAGE_SYSTEM, time(NULL));
			
		g_free(spawn_cmd);
		g_free(cmd_stdout);

		return FALSE;
	}
	
	g_free(cmd_stdout);
	g_free(spawn_cmd);

	return TRUE;
}

static PurpleCmdRet
se_cmd_cb(PurpleConversation *conv, const gchar *cmd, gchar **args, gchar **error,
			gpointer data)
{
	gboolean send = FALSE;
	char *string = args[0];

	if(string && !strncmp(string, "-o ", 3)) {
		send = TRUE;
		string += 3;
	}

	if(se_do_action(conv, string, send))
		return PURPLE_CMD_RET_OK;
	else
		return PURPLE_CMD_RET_FAILED;
}

static void
se_sending_msg_helper(PurpleConversation *conv, char **message)
{
	char *string = *message, *strip;
	gboolean send = TRUE;

	if(conv == NULL) return;

	strip = purple_markup_strip_html(string);

	if(*strip != '!') {
		g_free(strip);
		return;
	}

	*message = NULL;

	g_free(string);

	/* this is refactored quite a bit to simplify things since sending-im-msg
	 * allows changing the text that will be sent by simply giving *message a
	 * new pointer */
	if(strncmp(strip, "!!!", 3) == 0) {
		char *new_msg, *conv_sys_msg;
		
		new_msg = g_strdup(strip + 2);

		*message = new_msg;

		/* I really want to eventually make this cleaner, like by making it
		 * change the actual message that gets printed to the conv window... */
		conv_sys_msg = g_strdup_printf(_("The following text was sent: %s"),
				new_msg);

		purple_conversation_write(conv, NULL, conv_sys_msg, PURPLE_MESSAGE_SYSTEM,
				time(NULL));

		g_free(strip);
		g_free(conv_sys_msg);

		return;
	}

	if(*(strip + 1) == '!')
		send = FALSE;

	if(send)
		se_do_action(conv, strip + 1, send);
	else
		se_do_action(conv, strip + 2, send);

	g_free(strip);
}

static void
se_sending_chat_msg_cb(PurpleAccount *account, char **message, int id)
{
	PurpleConversation *conv;
	conv = purple_find_chat(account->gc, id);
	se_sending_msg_helper(conv, message);
}

static void
se_sending_im_msg_cb(PurpleAccount *account, const char *who, char **message)
{
	PurpleConversation *conv;
	conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, who, account);
	se_sending_msg_helper(conv, message);
}

static gboolean
se_load(PurplePlugin *plugin) {
#ifndef _WIN32
	struct passwd *pw = NULL;
#endif /* _WIN32 */
	const gchar *help = _("exec [-o] &lt;command&gt;, runs the command.\n"
						"If the -o flag is used then output is sent to the"
						"current conversation; otherwise it is printed to the "
						"current text box.");

	se_cmd = purple_cmd_register("exec", "s", PURPLE_CMD_P_PLUGIN,
								PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT, NULL,
								se_cmd_cb, help, NULL);

	/* these signals are needed for the bangexec features we took on */
	purple_signal_connect(purple_conversations_get_handle(), "sending-im-msg", plugin,
				PURPLE_CALLBACK(se_sending_im_msg_cb), NULL);
	purple_signal_connect(purple_conversations_get_handle(), "sending-chat-msg", plugin,
				PURPLE_CALLBACK(se_sending_chat_msg_cb), NULL);

	/* this gets the user's shell.  If this is built on Windows, force cmd.exe,
	 * which will make this plugin not work on Windows 98/ME */
#ifdef _WIN32
	shell = g_strdup("cmd.exe");
#else
	pw = getpwuid(getuid());
	
	if(pw)
		if(pw->pw_shell)
			shell = g_strdup(pw->pw_shell);
		else
			shell = g_strdup("/bin/sh");
	else
		shell = g_strdup("/bin/sh");
#endif /* _WIN32 */

	return TRUE;
}

static gboolean
se_unload(PurplePlugin *plugin) {
	purple_cmd_unregister(se_cmd);

	g_free(shell);

	return TRUE;
}

static PurplePluginInfo se_info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,								/* type				*/
	NULL,												/* ui requirement	*/
	0,													/* flags			*/
	NULL,												/* dependencies		*/
	PURPLE_PRIORITY_DEFAULT,								/* priority			*/
	"core-plugin_pack-slashexec",						/* id				*/
	"/exec",											/* name				*/
	PP_VERSION,											/* version			*/
	NULL,												/* summary			*/
	NULL,												/* description		*/

	"Gary Kramlich <amc_grim@users.sf.net>, "
	"Peter Lawler <bleeter from users.sf.net>, "
	"Daniel Atallah, Sadrul Habib Chowdhury <sadrul@users.sf.net>, "
	"John Bailey <rekkanoryo@users.sf.net>",			/* authors			*/

	PP_WEBSITE,											/* homepage			*/
	se_load,											/* load				*/
	se_unload,											/* unload			*/
	NULL,												/* destroy			*/
	NULL,												/* ui info			*/
	NULL,												/* extra info		*/
	NULL,												/* actions info		*/
	NULL
};

static void
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */
	se_info.summary = _("/exec a la UNIX IRC CLI");
	se_info.description = _("A plugin that adds the /exec command line"
							" interpreter like most UNIX/Linux IRC"
							" clients have.  Also included is the ability to"
							" execute commands with an exclamation point"
							" (!uptime, for instance).\n");
}

PURPLE_INIT_PLUGIN(slashexec, init_plugin, se_info)

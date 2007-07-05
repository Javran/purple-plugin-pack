/**
 * @file irc-more.c A couple of additional IRC features.
 *
 * Copyright (C) 2007 Sadrul Habib Chowdhury <sadrul@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
# include "../pp_config.h"
#endif

#define PURPLE_PLUGINS

#include <plugin.h>
#include <accountopt.h>
#include <conversation.h>
#include <version.h>

#include <string.h>

/* Pack/Local headers */
#include "../common/pp_internal.h"

#define CTCP_REPLY    purple_account_get_string(account, "ctcp-message", "Purple IRC")
#define PART_MESSAGE  purple_account_get_string(account, "part-message", "Leaving.")
#define QUIT_MESSAGE  purple_account_get_string(account, "quit-message", "Leaving.")

static void
irc_sending_text(PurpleConnection *gc, char **msg, gpointer null)
{
	PurpleAccount *account = purple_connection_get_account(gc);
	char **old = msg;

#define MATCHES(string)   !strncmp(*msg, string, sizeof(string) - 1)

	if (MATCHES("QUIT ")) {
		char *message = strchr(*msg, ':');
		if (!message || !strcmp(message + 1, "Leaving.\r\n")) {
			*msg = g_strdup_printf("QUIT :%s\r\n", QUIT_MESSAGE);
		}
	} else if (MATCHES("PART ")) {
		char *message = strchr(*msg, ':');
		if (message)
			return;   /* The user did give some part message. Do not use the default one. */
		message = strchr(*msg, '\r');
		*message = '\0';
		*msg = g_strdup_printf("%s :%s\r\n", *msg, PART_MESSAGE);
	} else if (MATCHES("NOTICE ")) {
		char *version = strstr(*msg, ":\001VERSION ");
		if (!version)
			return;
		*version = '\0';
		*msg = g_strdup_printf("%s:\001VERSION %s\001\r\n", *msg, CTCP_REPLY);
	}
	if (msg != old)
		g_free(*old);
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	PurplePlugin *prpl = purple_find_prpl("prpl-irc");
	PurplePluginProtocolInfo *info;
	PurpleAccountOption *option;

	if (!prpl)
		return FALSE;
	purple_signal_connect(prpl, "irc-sending-text", plugin,
				G_CALLBACK(irc_sending_text), NULL);

	info = PURPLE_PLUGIN_PROTOCOL_INFO(prpl);

	option = purple_account_option_string_new(_("Default Quit Message"), "quit-message", "Leaving.");
	info->protocol_options = g_list_append(info->protocol_options, option);

	option = purple_account_option_string_new(_("Default Part Message"), "part-message", "Leaving.");
	info->protocol_options = g_list_append(info->protocol_options, option);

	option = purple_account_option_string_new(_("CTCP Version reply"), "ctcp-message", "Purple IRC");
	info->protocol_options = g_list_append(info->protocol_options, option);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	/* XXX: Remove the options from here, or may be not. */
	return TRUE;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,
	"irc-more",
	N_("IRC More"),
	VERSION,
	N_("Adds a couple of options for IRC: Customized default quit message, and CTCP version reply."),
	N_("Adds a couple of options for IRC: Customized default quit message, and CTCP version reply."),
	"Sadrul H Chowdhury <sadrul@users.sourceforge.net>",
	PP_WEBSITE,
	plugin_load,
	plugin_unload,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,NULL,NULL,NULL
};

static void
init_plugin(PurplePlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */
}

PURPLE_INIT_PLUGIN(irc_more, init_plugin, info)


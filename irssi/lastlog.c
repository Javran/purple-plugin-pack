/*
 * irssi - Implements several irssi features for Purple
 * Copyright (C) 2005-2008 Gary Kramlich <grim@reaperworld.com>
 * Copyright (C) 2006-2008 John Bailey <rekkanoryo@rekkanoryo.org>
 * Copyright (C) 2006-2008 Sadrul Habib Chowdhury <sadrul@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02111-1301, USA.
 */

/* If you can't figure out what this line is for, DON'T TOUCH IT. */
#include "../common/pp_internal.h"

#include <string.h>

#include <plugin.h>
#include <cmds.h>
#include <util.h>

#include <gtkconv.h>
#include <gtkimhtml.h>

#include "lastlog.h"

/******************************************************************************
 * Globals
 *****************************************************************************/
static PurpleCmdId irssi_lastlog_cmd = 0;

/******************************************************************************
 * Helpers
 *****************************************************************************/
static void
irssi_lastlog(PurpleConversation *c, const gchar *needle) {
	PidginConversation *gtkconv = c->ui_data;
	int i;
	GString *result;
	char **lines;

	/* let's avoid some warnings on anal C compilers like mipspro cc */
	result = g_string_new(NULL);
	lines = gtk_imhtml_get_markup_lines(GTK_IMHTML(gtkconv->imhtml));

	/* XXX: This will include all messages, including the output of the
	 * history plugin, system messages, timestamps etc. This might be
	 * undesirable. A better solution will probably be considerably more
	 * complex.
	 */

	for (i = 0; lines[i]; i++) {
		char *strip = purple_markup_strip_html(lines[i]);
		if (strstr(strip, needle)) {
			result = g_string_append(result, lines[i]);
			result = g_string_append(result, "<br>");
		}

		g_free(strip);
	}

	/* XXX: This should probably be moved into outputting directly in the
	 * conversation window.
	 */
	purple_notify_formatted(gtkconv, _("Lastlog"), _("Lastlog output"), NULL,
						  result->str, NULL, NULL);

	g_string_free(result, TRUE);
	g_strfreev(lines);
}

static PurpleCmdRet
irssi_lastlog_cmd_cb(PurpleConversation *conv, const gchar *cmd, gchar **args,
					 gchar **error, void *data)
{
	irssi_lastlog(conv, args[0]);

	return PURPLE_CMD_RET_OK;
}

/******************************************************************************
 * "API"
 *****************************************************************************/
void
irssi_lastlog_init(PurplePlugin *plugin) {
	const gchar *help;

	if(irssi_lastlog_cmd != 0)
		return;

	/* XXX: Translators: DO NOT TRANSLATE "lastlog" or the HTML tags below */
	help = _("<pre>lastlog &lt;string&gt;: Shows, from the current "
			 "conversation's history, all messages containing the word or "
			 "words specified in string.  It will be an exact match, "
			 "including whitespace and special characters.");

	/*
	 * registering the /lastlog command so that it will take an arbitrary
	 * number of arguments 1 or greater, matched as a single string, mark it
	 * as a plugin-priority command, make it functional in both IMs and chats,
	 * set it such that it is NOT protocol-specific, specify the callback and
	 * the help string, and set no user data to pass to the callback
	 */
	irssi_lastlog_cmd =
		purple_cmd_register("lastlog", "s", PURPLE_CMD_P_PLUGIN,
						  PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT, NULL,
						  PURPLE_CMD_FUNC(irssi_lastlog_cmd_cb), help, NULL);
}

void
irssi_lastlog_uninit(PurplePlugin *plugin) {
	purple_cmd_unregister(irssi_lastlog_cmd);

	irssi_lastlog_cmd = 0;
}


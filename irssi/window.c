/*
 * irssi - Implements several irssi features for Purple
 * Copyright (C) 2005-2007 Gary Kramlich <grim@reaperworld.com>
 * Copyright (C) 2007 John Bailey <rekkanoryo@rekkanoryo.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

/* pp_config.h provides necessary definitions that help us find/do stuff */
#ifdef HAVE_CONFIG_H
# include "../pp_config.h"
#endif

#include <gtk/gtk.h>

#include <cmds.h>
#include <conversation.h>
#include <plugin.h>

#include <gtkconv.h>

#include "../common/pp_internal.h"
#include "window.h"

/******************************************************************************
 * Globals
 *****************************************************************************/
static PurpleCmdId irssi_window_cmd_id = 0;
static PurpleCmdId irssi_win_cmd_id = 0;

/******************************************************************************
 * Helpers
 *****************************************************************************/
static gboolean
irssi_window_close_cb(PurpleConversation *c) {
	/* this gets called from a conversation since the conversation must exist
	 * until all of the commands are processed, and the output is output.
	 */

	purple_conversation_destroy(c);

	return FALSE;
}

static PurpleCmdRet
irssi_window_cmd(PurpleConversation *conv, const gchar *sub_cmd,
				 gchar **error)
{
	PidginConversation *gtkconv;
	PidginWindow *win;
	gint cur;

	gtkconv = PIDGIN_CONVERSATION(conv);
	win = gtkconv->win;
	cur = gtk_notebook_get_current_page(GTK_NOTEBOOK(win->notebook));

	/* if the sub_cmd is a number, or starts with one, assume the user wants
	 * to switch to a specific numbered tab */
	if(g_ascii_isdigit(*sub_cmd)) {
		gint tab = atoi(sub_cmd) - 1; /* index starts at zero */

		if(tab < 0) {
			*error = g_strdup(_("Invalid window specified."));

			return PURPLE_CMD_RET_FAILED;
		}

		if(tab < pidgin_conv_window_get_gtkconv_count(win))
			gtk_notebook_set_current_page(GTK_NOTEBOOK(win->notebook), tab);

		return PURPLE_CMD_RET_OK;
	}

	if(!g_ascii_strcasecmp(sub_cmd, "close")) {
		g_timeout_add(50, (GSourceFunc)irssi_window_close_cb, conv);

		return PURPLE_CMD_RET_OK;
	} else if(!g_ascii_strcasecmp(sub_cmd, "next") ||
			  !g_ascii_strcasecmp(sub_cmd, "right"))
	{
		if(!pidgin_conv_window_get_gtkconv_at_index(win, cur + 1)) {
			/* wrap around... */
			gtk_notebook_set_current_page(GTK_NOTEBOOK(win->notebook), 0);
		} else {
			/* move normally */
			gtk_notebook_set_current_page(GTK_NOTEBOOK(win->notebook),
										  cur + 1);
		}

		return PURPLE_CMD_RET_OK;
	} else if(!g_ascii_strcasecmp(sub_cmd, "previous") ||
			  !g_ascii_strcasecmp(sub_cmd, "prev") ||
			  !g_ascii_strcasecmp(sub_cmd, "left"))
	{
		if(!pidgin_conv_window_get_gtkconv_at_index(win, cur - 1)) {
			/* wrap around... */
			gtk_notebook_set_current_page(GTK_NOTEBOOK(win->notebook), -1);
		} else {
			gtk_notebook_set_current_page(GTK_NOTEBOOK(win->notebook),
										  cur - 1);
		}

		return PURPLE_CMD_RET_OK;
	} else {
		*error = g_strdup(_("Invalid argument!"));

		return PURPLE_CMD_RET_FAILED;
	}

	*error = g_strdup(_("Unknown Error!"));
	return PURPLE_CMD_RET_FAILED;
}

static PurpleCmdRet
irssi_window_cmd_cb(PurpleConversation *conv, const gchar *cmd, gchar **args,
					gchar **error, void *data)
{
	return irssi_window_cmd(conv, args[0], error);
}

/******************************************************************************
 * "API"
 *****************************************************************************/
void
irssi_window_init(PurplePlugin *plugin) {
	const gchar *help;

	if(irssi_window_cmd_id != 0 || irssi_win_cmd_id != 0)
		return;
	
	/*
	 * XXX: Translators: DO NOT TRANSLATE the first occurance of the word
	 * "window" below, or "close", "next", "previous", "left", or "right"
	 * at the *beginning* of the lines below!  The options to /window are
	 * NOT going to be translatable.  Also, please don't translate the HTML
	 * tags.
	 */
	help = _("<pre>window &lt;option&gt;: Operations for windows (tabs).  "
			 "Valid options are:\n"
			 "close - closes the current conversation\n"
			 "next - move to the next conversation\n"
			 "previous - move to the previous conversation\n"
			 "left - move one conversation to the left\n"
			 "right - move one conversation to the right\n"
			 "&lt;number&gt; - go to tab <number>\n"
			 "</pre>");

	irssi_window_cmd_id =
		purple_cmd_register("window", "w", PURPLE_CMD_P_PLUGIN,
						  PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT, NULL, 
						  PURPLE_CMD_FUNC(irssi_window_cmd_cb), help, NULL);

	/* same thing as above, except for the /win command */
	help = _("<pre>win: THis command is synonymous with /window.  Try /help "
			 "window for further details.</pre>");

	irssi_win_cmd_id =
		purple_cmd_register("win", "w", PURPLE_CMD_P_PLUGIN, 
						  PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT, NULL,
						  PURPLE_CMD_FUNC(irssi_window_cmd_cb), help, NULL);
}

void
irssi_window_uninit(PurplePlugin *plugin) {
	if(irssi_window_cmd_id == 0 || irssi_win_cmd_id == 0)
		return;

	purple_cmd_unregister(irssi_window_cmd_id);
	purple_cmd_unregister(irssi_win_cmd_id);
}


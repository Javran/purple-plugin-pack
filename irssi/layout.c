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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02111-1301, USA.
 */

#include "../common/pp_internal.h"

#include <blist.h>
#include <cmds.h>
#include <plugin.h>

#include <gtkblist.h>
#include <gtkconv.h>

#include "layout.h"

#define IRSSI_LAYOUT_SETTING	"irssi::layout"

#define SETTING_TO_INTS(s, i1, i2) { \
	(i1) = (s) & 0x3ff; \
	(i2) = (s) >> 10; \
}

#define SETTING_FROM_INTS(i1, i2)	(((i1) << 10 ) + (i2))

/******************************************************************************
 * Globals
 *****************************************************************************/
static PurpleCmdId irssi_layout_cmd_id = 0;

/******************************************************************************
 * Helpers
 *****************************************************************************/
static PurpleBlistNode *
irssi_layout_get_node_from_conv(PurpleConversation *conv) {
	PurpleBlistNode *node = NULL;

	/* this is overkill for now, but who knows, we _may_ need it later */

	switch(conv->type) {
		case PURPLE_CONV_TYPE_CHAT:
			node = (PurpleBlistNode *)purple_blist_find_chat(conv->account,
														 conv->name);

			break;
		case PURPLE_CONV_TYPE_IM:
			node = (PurpleBlistNode *)purple_find_buddy(conv->account, conv->name);

			break;

		default:
			node = NULL;
	}
	
	return node;
}

static PurpleConversation *
irssi_layout_get_conv_from_node(PurpleBlistNode *node, gboolean create) {
	PurpleAccount *account = NULL;
	PurpleConversation *conv = NULL;
	PurpleConversationType ctype = PURPLE_CONV_TYPE_UNKNOWN;

	const gchar *name = NULL;

	switch(node->type) {
		case PURPLE_BLIST_CHAT_NODE: {
			PurpleChat *chat = (PurpleChat *)node;

			ctype = PURPLE_CONV_TYPE_CHAT;
			name = purple_chat_get_name(chat);
			account = chat->account;

			break;
		}
		case PURPLE_BLIST_BUDDY_NODE: {
			PurpleBuddy *buddy = (PurpleBuddy *)node;
		
			ctype = PURPLE_CONV_TYPE_IM;
			name = buddy->name;
			account = buddy->account;

			break;
		}
		default:
			return NULL;
			break;
	}

	conv = purple_find_conversation_with_account(ctype, name, account);

	if(!conv && create) {
		conv = purple_conversation_new(ctype, account, name);

		/* dirty hack alert! */
		if(ctype == PURPLE_BLIST_CHAT_NODE) {
			PurpleChat *chat = (PurpleChat *)node;

			PURPLE_CONV_CHAT(conv)->left = TRUE;
			serv_join_chat(account->gc, chat->components);
		}
	}

	return conv;
}

static gint
irssi_layout_get_setting(PidginConversation *gtkconv) {
	PurpleConversation *conv = gtkconv->active_conv;
	PurpleBlistNode *node = NULL;
	gint ret = 0;

	node = irssi_layout_get_node_from_conv(conv);

	if(node)
		ret = purple_blist_node_get_int(node, IRSSI_LAYOUT_SETTING);
	
	return ret;
}

static void
irssi_layout_reset(void) {
	PurpleBlistNode *node = purple_blist_get_root();

	for(; node; node = purple_blist_node_next(node, TRUE))
		purple_blist_node_remove_setting(node, IRSSI_LAYOUT_SETTING);
}

static void
irssi_layout_save(void) {
	PurpleBlistNode *node = NULL;
	GList *wins = NULL;
	gint i, j;

	/* reset the previous layouts if any exist */
	irssi_layout_reset();
	
	/* now save the layout... */
	wins = pidgin_conv_windows_get_list();

	for(i = 1; wins; wins = wins->next, i++) {
		PidginWindow *win = wins->data;
		GList *convs = pidgin_conv_window_get_gtkconvs(win);

		for(j = 1; convs; convs = convs->next, j++) {
			PidginConversation *gtkconv = convs->data;
			PurpleConversation *conv = gtkconv->active_conv;

			node = irssi_layout_get_node_from_conv(conv);

			if(node)
				purple_blist_node_set_int(node, IRSSI_LAYOUT_SETTING,
										SETTING_FROM_INTS(i, j));
		}
	}
}

static void
irssi_layout_load(void) {
	PurpleConversation *conv = NULL;
	PurpleBlistNode *node;

	PidginConversation *gtkconv = NULL;
	PidginWindow *window = NULL;

	GList *convs = NULL, *settings = NULL, *wins = NULL;

	gint current = 1;

	node = purple_blist_get_root();

	/* build our GList's with the conversation and the setting */
	for(; node; node = purple_blist_node_next(node, FALSE)) {
		gint setting = purple_blist_node_get_int(node, IRSSI_LAYOUT_SETTING);

		if(setting == 0)
			continue;

		conv = irssi_layout_get_conv_from_node(node, FALSE);

		if(conv) {
			convs = g_list_prepend(convs, conv);
			settings = g_list_prepend(settings, GINT_TO_POINTER(setting));
		}
	}

	/* now restore the layout */

	/* we start by looping until all of our settings are handled */
	while(convs) {
		GList *l1 = NULL, *l2 = NULL;

		/* now go through the list and make sure we put the conversations in
		 * the correct windows.
		 */
		for(l1 = convs, l2 = settings; l1; ) {
			GList *s = NULL;
			gint win, pos, setting;

			setting = GPOINTER_TO_INT(l2->data);
			SETTING_TO_INTS(setting, pos, win);

			if(win != current)
				continue;

			conv = l1->data;
			gtkconv = conv->ui_data;

			/* pop of the nodes we just handled but hold our places and update
			 * the head pointers.
			 */
			/* convs */
			s = l1;
			l1 = l1->next;
			convs = g_list_delete_link(convs, s);

			/* settings */
			s = l2;
			l2 = l2->next;
			settings = g_list_delete_link(settings, s);

			/* now find the actual window this should go into */
			wins = pidgin_conv_windows_get_list();
			window = g_list_nth_data(wins, win - 1);

			if(!window) {
				/* make dat der dun winda */
				window = pidgin_conv_window_new();
			}

			/* add the conversation to the window */
			if(gtkconv->win != window) {
				pidgin_conv_window_remove_gtkconv(gtkconv->win, gtkconv);
				pidgin_conv_window_add_gtkconv(window, gtkconv);
			}
		}

		current++;
	}

	/* All the conversations are in the correct windows.  Now we make sure
	 * they're in their right positions.
	 */
	for(wins = pidgin_conv_windows_get_list(); wins; wins = wins->next) {
		gint count, i, pos, position;
		gint w; /* junk var */

		window = wins->data;
		count = pidgin_conv_window_get_gtkconv_count(window);

		if(count <= 1)
			continue;

		for(position = 1; position < count; position++) {
			gtkconv = pidgin_conv_window_get_gtkconv_at_index(window, position);
			pos = irssi_layout_get_setting(gtkconv);

			SETTING_TO_INTS(pos, pos, w);
			if(pos <= 0)
				continue;

			/* this could probably use tweaking, but it _should_ work */
			for(i = pos; i < position; i++) {
				PidginConversation *gtkconv2 = NULL;
				gint p;
				
				gtkconv2 =
					pidgin_conv_window_get_gtkconv_at_index(window, i);

				p = irssi_layout_get_setting(gtkconv2);
				if(p <= 0 || p <= pos)
					continue;

				gtk_notebook_reorder_child(GTK_NOTEBOOK(window->notebook),
										   gtkconv->tab_cont, i);
			}
		}
	}
}

static PurpleCmdRet
irssi_layout_cmd_cb(PurpleConversation *conv, const gchar *cmd, gchar **args,
					gchar **error, void *data)
{
	const gchar *sub_cmd = args[0];

	if(!g_ascii_strcasecmp(sub_cmd, "load")) {
		irssi_layout_load();
	} else if(!g_ascii_strcasecmp(sub_cmd, "save")) {
		irssi_layout_save();
	} else if(!g_ascii_strcasecmp(sub_cmd, "reset")) {
		irssi_layout_reset();
	}

	return PURPLE_CMD_RET_OK;
}

/******************************************************************************
 * "API"
 *****************************************************************************/
void
irssi_layout_init(PurplePlugin *plugin) {
	const gchar *help;

	if(irssi_layout_cmd_id != 0)
		return;
	/*
	 * XXX: Translators: DO NOT TRANSLATE the first "layout" or the "\nsave"
	 * or "reset" at the beginning of the last line below, or the HTML tags.
	 */
	help = _("<pre>layout &lt;save|reset&gt;: Remember the layout of the "
			 "current conversations to reopen them when Purple is restarted.\n"
			 "save - saves the current layout\n"
			 "reset - clears the current saved layout\n"
			 "</pre>");

	irssi_layout_cmd_id =
		purple_cmd_register("layout", "w", PURPLE_CMD_P_PLUGIN,
						  PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT, NULL,
						  PURPLE_CMD_FUNC(irssi_layout_cmd_cb), help, NULL);
}

void
irssi_layout_uninit(PurplePlugin *plugin) {
	if(irssi_layout_cmd_id == 0)
		return;
	
	purple_cmd_unregister(irssi_layout_cmd_id);
}


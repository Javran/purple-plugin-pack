/*
 * irssi - Implements several irssi features for Gaim
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

/* gpp_config.h provides necessary definitions that help us find/do stuff */
#ifdef HAVE_CONFIG_H
# include "../gpp_config.h"
#endif

#include <blist.h>
#include <cmds.h>
#include <plugin.h>

#include <gtkblist.h>
#include <gtkconv.h>

#include "../common/i18n.h"

#define IRSSI_LAYOUT_SETTING	"irssi::layout"

#define SETTING_TO_INTS(s, i1, i2) { \
	(i1) = (s) & 0x3ff; \
	(i2) = (s) >> 10; \
}

#define SETTING_FROM_INTS(i1, i2)	(((i1) << 10 ) + (i2))

/******************************************************************************
 * Globals
 *****************************************************************************/
static GaimCmdId irssi_layout_cmd_id = 0;

/******************************************************************************
 * Helpers
 *****************************************************************************/
static GaimBlistNode *
irssi_layout_get_node_from_conv(GaimConversation *conv) {
	GaimBlistNode *node = NULL;

	/* this is overkill for now, but who knows, we _may_ need it later */

	switch(conv->type) {
		case GAIM_CONV_TYPE_CHAT:
			node = (GaimBlistNode *)gaim_blist_find_chat(conv->account,
														 conv->name);

			break;
		case GAIM_CONV_TYPE_IM:
			node = (GaimBlistNode *)gaim_find_buddy(conv->account, conv->name);

			break;

		default:
			node = NULL;
	}
	
	return node;
}

static GaimConversation *
irssi_layout_get_conv_from_node(GaimBlistNode *node, gboolean create) {
	GaimAccount *account = NULL;
	GaimConversation *conv = NULL;
	GaimConversationType ctype = GAIM_CONV_TYPE_UNKNOWN;

	const gchar *name = NULL;

	switch(node->type) {
		case GAIM_BLIST_CHAT_NODE: {
			GaimChat *chat = (GaimChat *)node;

			ctype = GAIM_CONV_TYPE_CHAT;
			name = gaim_chat_get_name(chat);
			account = chat->account;

			break;
		}
		case GAIM_BLIST_BUDDY_NODE: {
			GaimBuddy *buddy = (GaimBuddy *)node;
		
			ctype = GAIM_CONV_TYPE_IM;
			name = buddy->name;
			account = buddy->account;

			break;
		}
		default:
			return NULL;
			break;
	}

	conv = gaim_find_conversation_with_account(ctype, name, account);

	if(!conv && create) {
		conv = gaim_conversation_new(ctype, account, name);

		/* dirty hack alert! */
		if(ctype == GAIM_BLIST_CHAT_NODE) {
			GaimChat *chat = (GaimChat *)node;

			GAIM_CONV_CHAT(conv)->left = TRUE;
			serv_join_chat(account->gc, chat->components);
		}
	}

	return conv;
}

static gint
irssi_layout_get_setting(GaimGtkConversation *gtkconv) {
	GaimConversation *conv = gtkconv->active_conv;
	GaimBlistNode *node = NULL;
	gint ret = 0;

	node = irssi_layout_get_node_from_conv(conv);

	if(node)
		ret = gaim_blist_node_get_int(node, IRSSI_LAYOUT_SETTING);
	
	return ret;
}

static void
irssi_layout_reset(void) {
	GaimBlistNode *node = gaim_blist_get_root();

	for(; node; node = gaim_blist_node_next(node, TRUE))
		gaim_blist_node_remove_setting(node, IRSSI_LAYOUT_SETTING);
}

static void
irssi_layout_save(void) {
	GaimBlistNode *node = NULL;
	GList *wins = NULL;
	gint i, j;

	/* reset the previous layouts if any exist */
	irssi_layout_reset();
	
	/* now save the layout... */
	wins = gaim_gtk_conv_windows_get_list();

	for(i = 1; wins; wins = wins->next, i++) {
		GaimGtkWindow *win = wins->data;
		GList *convs = gaim_gtk_conv_window_get_gtkconvs(win);

		for(j = 1; convs; convs = convs->next, j++) {
			GaimGtkConversation *gtkconv = convs->data;
			GaimConversation *conv = gtkconv->active_conv;

			node = irssi_layout_get_node_from_conv(conv);

			if(node)
				gaim_blist_node_set_int(node, IRSSI_LAYOUT_SETTING,
										SETTING_FROM_INTS(i, j));
		}
	}
}

static void
irssi_layout_load(void) {
	GaimConversation *conv = NULL;
	GaimBlistNode *node;

	GaimGtkConversation *gtkconv = NULL;
	GaimGtkWindow *window = NULL;

	GList *convs = NULL, *settings = NULL, *wins = NULL;

	gint current = 1;

	node = gaim_blist_get_root();

	/* build our GList's with the conversation and the setting */
	for(; node; node = gaim_blist_node_next(node, FALSE)) {
		gint setting = gaim_blist_node_get_int(node, IRSSI_LAYOUT_SETTING);

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
			wins = gaim_gtk_conv_windows_get_list();
			window = g_list_nth_data(wins, win - 1);

			if(!window) {
				/* make dat der dun winda */
				window = gaim_gtk_conv_window_new();
			}

			/* add the conversation to the window */
			if(gtkconv->win != window) {
				gaim_gtk_conv_window_remove_gtkconv(gtkconv->win, gtkconv);
				gaim_gtk_conv_window_add_gtkconv(window, gtkconv);
			}
		}

		current++;
	}

	/* All the conversations are in the correct windows.  Now we make sure
	 * they're in their right positions.
	 */
	for(wins = gaim_gtk_conv_windows_get_list(); wins; wins = wins->next) {
		gint count, i, pos, position;
		gint w; /* junk var */

		window = wins->data;
		count = gaim_gtk_conv_window_get_gtkconv_count(window);

		if(count <= 1)
			continue;

		for(position = 1; position < count; position++) {
			gtkconv = gaim_gtk_conv_window_get_gtkconv_at_index(window, position);
			pos = irssi_layout_get_setting(gtkconv);

			SETTING_TO_INTS(pos, pos, w);
			if(pos <= 0)
				continue;

			/* this could probably use tweaking, but it _should_ work */
			for(i = pos; i < position; i++) {
				GaimGtkConversation *gtkconv2 = NULL;
				gint p;
				
				gtkconv2 =
					gaim_gtk_conv_window_get_gtkconv_at_index(window, i);

				p = irssi_layout_get_setting(gtkconv2);
				if(p <= 0 || p <= pos)
					continue;

				gtk_notebook_reorder_child(GTK_NOTEBOOK(window->notebook),
										   gtkconv->tab_cont, i);
			}
		}
	}
}

static GaimCmdRet
irssi_layout_cmd_cb(GaimConversation *conv, const gchar *cmd, gchar **args,
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

	return GAIM_CMD_RET_OK;
}

/******************************************************************************
 * "API"
 *****************************************************************************/
void
irssi_layout_init(GaimPlugin *plugin) {
	const gchar *help;

	if(irssi_layout_cmd_id != 0)
		return;
	/*
	 * XXX: Translators: DO NOT TRANSLATE the first "layout" or the "\nsave"
	 * or "reset" at the beginning of the last line below, or the HTML tags.
	 */
	help = _("<pre>layout &lt;save|reset&gt;: Remember the layout of the "
			 "current conversations to reopen them when Gaim is restarted.\n"
			 "save - saves the current layout\n"
			 "reset - clears the current saved layout\n"
			 "</pre>");

	irssi_layout_cmd_id =
		gaim_cmd_register("layout", "w", GAIM_CMD_P_PLUGIN,
						  GAIM_CMD_FLAG_IM | GAIM_CMD_FLAG_CHAT, NULL,
						  GAIM_CMD_FUNC(irssi_layout_cmd_cb), help, NULL);
}

void
irssi_layout_uninit(GaimPlugin *plugin) {
	if(irssi_layout_cmd_id == 0)
		return;
	
	gaim_cmd_unregister(irssi_layout_cmd_id);
}


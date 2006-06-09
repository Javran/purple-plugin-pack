/*
 * Gaim Plugin Pack
 * Copyright (C) 2003-2005
 * See AUTHORS for a list of all authors
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

#ifndef GPP_COMPAT_H
#define GPP_COMPAT_H

/* we need the GAIM_VERSION_CHECK macro to make these macros below work */
#include <version.h>

#if GAIM_VERSION_CHECK(2,0,0)

# define gpp_find_conversation(type, name, account) \
	gaim_find_conversation_with_account((type), (name), (account))

# define gpp_conv_switch(win, conv) \
	gaim_conv_window_switch_conversation((win), (conv))

# define gpp_prefs_connect_callback(plugin, pref, cb, data) \
	gaim_prefs_connect_callback((plugin), (pref), (cb), (data))

# define gpp_chat_get_name(chat) gaim_chat_get_name((chat))

# define gpp_get_warning_level(buddy) \
	gaim_presence_get_warning_level((buddy)->presence)

# define gpp_mkstemp(path) gaim_mkstemp((path), FALSE)

# define gpp_create_prpl_icon(account) gaim_gtk_create_prpl_icon((account))

# define gpp_presence_is_valid(account) \
	gaim_presence_is_available((account)->presence)

# define gpp_conversation_get_type(conv) gaim_conversation_get_type(conv->active_conv)

# define gpp_add_buddy(account, buddy) gaim_account_add_buddy((account), (buddy))

# define gpp_add_buddies(account, buddies) gaim_account_add_buddies(account, buddies)

#else /* GAIM_VERSION_CHECK(2,0,0) */

#define GaimMenuAction GaimBlistNodeAction

# define gpp_find_conversation(type, name, account) \
	gaim_find_conversation_with_account((name), (account))

# define GAIM_CONV_ANY	(4)

# define gpp_conv_switch(win, conv) \
	gaim_conv_window_switch_conversation(win, gaim_conversation_get_index((conv)))

# define gaim_menu_action_new(string, cb, data, children) \
	gaim_blist_node_action_new((string), (void *)(cb), (data))

# define gpp_prefs_connect_callback(plugin, pref, cb, data) \
	gaim_prefs_connect_callback((pref), (void *)(cb), (data))

# define gpp_chat_get_name(chat) gaim_chat_get_display_name((chat))

# define gpp_get_warning_level(buddy) (buddy)->evil

# define gpp_mkstemp(path) gaim_mkstemp((path))

# define gpp_create_prpl_icon(account) create_prpl_icon((account))

# define gpp_presence_is_valid(account) (!(account)->gc->away_state)

# define gpp_add_buddy(account, buddy) \
	serv_add_buddy(gaim_account_get_connection((account)), (buddy))

# define gpp_add_buddies(account, buddies) \
	serv_add_buddies(gaim_account_get_connection(account), buddies)

# define GAIM_CONV_TYPE_IM GAIM_CONV_IM

# define GAIM_CONV_TYPE_CHAT GAIM_CONV_CHAT

# define GAIM_CONV_TYPE_ANY	GAIM_CONV_ANY

# define gpp_conversation_get_type(conv) gaim_conversation_get_type(conv)

# define GaimGtkWindow GaimConvWindow

# define gaim_gtk_conv_window_last_with_type(type) \
	gaim_get_last_window_with_type((type))

# define gaim_gtk_conv_windows_get_list() gaim_get_windows()

# define gaim_gtk_conv_window_add_gtkconv(win, conv) \
	gaim_conv_window_add_conversation((win), (conv))

# define gaim_gtk_conv_window_show(win) gaim_conv_window_show((win))

# define gaim_gtk_conv_window_add_gtkconv(win, conv) \
	gaim_conv_window_add_conversation((win), (conv))

# define gaim_gtk_conv_window_new() gaim_conv_window_new()

# define gaim_gtkconv_placement_add_fnc(id, name, func) \
	gaim_conv_placement_add_fnc((id), (name), (func))

# define gaim_gtkconv_placement_remove_fnc(id) \
	gaim_conv_placement_remove_fnc((id))

# define gaim_buddy_get_name(buddy) ((buddy)->name)

# define gaim_buddy_get_account(buddy) ((buddy)->account)

#endif /* GAIM_VERSION_CHECK(2,0,0) */

#endif /* GPP_COMPAT_H */

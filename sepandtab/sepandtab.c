/*
 * Extra conversation placement options for Gaim
 * Copyright (C) 2004 Gary Kramlich <amc_grim@users.sf.net>
 *
 * Gaim is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
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
#if HAVE_CONFIG_H
# include "../gpp_config.h"
#endif

#define GAIM_PLUGINS

#include <conversation.h>
#include <plugin.h>
#include <version.h>
#include <gtkplugin.h>
#if GAIM_VERSION_CHECK(2,0,0)
#include <gtkconv.h>
#include <gtkconvwin.h>
#endif

#include "../common/i18n.h"
#include "../common/gpp_compat.h"

static void
#if GAIM_VERSION_CHECK(2,0,0)
conv_placement_sep_ims_tab_chats(GaimGtkConversation *conv) {
#else
conv_placement_sep_ims_tab_chats(GaimConversation *conv) {
#endif
	GaimConversationType type;
	GaimGtkWindow *win = NULL;

	type = gpp_conversation_get_type(conv);
	win = gaim_gtk_conv_window_last_with_type(type);

	if(type == GAIM_CONV_TYPE_IM) {
		win = gaim_gtk_conv_window_new();

		gaim_gtk_conv_window_add_gtkconv(win, conv);
		gaim_gtk_conv_window_show(win);
	} else {
		if(!win)
			win = gaim_gtk_conv_window_new();

		gaim_gtk_conv_window_add_gtkconv(win, conv);
		gaim_gtk_conv_window_show(win);
	}
}

static void
#if GAIM_VERSION_CHECK(2,0,0)
conv_placement_sep_chats_tab_ims(GaimGtkConversation *conv) {
#else
conv_placement_sep_chats_tab_ims(GaimConversation *conv) {
#endif
	GaimConversationType type;
	GaimGtkWindow *win = NULL;

	type = gpp_conversation_get_type(conv);
	win = gaim_gtk_conv_window_last_with_type(type);

	if(type == GAIM_CONV_TYPE_CHAT) {
		win = gaim_gtk_conv_window_new();

		gaim_gtk_conv_window_add_gtkconv(win, conv);
		gaim_gtk_conv_window_show(win);
	} else {
		if(!win)
			win = gaim_gtk_conv_window_new();

		gaim_gtk_conv_window_add_gtkconv(win, conv);
		gaim_gtk_conv_window_show(win);
	}
}

static gboolean
plugin_load(GaimPlugin *plugin) {
	gaim_gtkconv_placement_add_fnc("sep-ims-tab-chats", _("Separate IM, group Chats"),
							    &conv_placement_sep_ims_tab_chats);
	gaim_gtkconv_placement_add_fnc("sep-chats-tab-ims", _("Separate Chats, group IMs"),
								&conv_placement_sep_chats_tab_ims);
	gaim_prefs_trigger_callback("/gaim/gtk/conversations/placement");

	return TRUE;
}

static gboolean
plugin_unload(GaimPlugin *plugin) {
	gaim_gtkconv_placement_remove_fnc("sep-ims-tab-chats");
	gaim_gtkconv_placement_remove_fnc("sep-chats-tab-ims");

	gaim_prefs_trigger_callback("/gaim/gtk/conversations/placement");

	return TRUE;
}

static GaimPluginInfo info = {
	GAIM_PLUGIN_MAGIC,
	GAIM_MAJOR_VERSION,
	GAIM_MINOR_VERSION,
	GAIM_PLUGIN_STANDARD,
	GAIM_GTK_PLUGIN_TYPE,
	0,
	NULL,
	GAIM_PRIORITY_DEFAULT,
	"gtk-plugin_pack-separate_and_tab",
	NULL,
	GPP_VERSION,
	NULL,
	NULL,
	"Gary Kramlich <amc_grim@users.sf.net>",
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
init_plugin(GaimPlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GPP_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GPP_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Separate And Tab");
	info.summary = _("Adds two placement functions for separating and tabbing");
	info.description = _("Adds two new placement functions.\n\n"
						 "One separates IMs and groups chats in tabs\n"
						 "The other separates chats and groups IMs in tabs");

}

GAIM_INIT_PLUGIN(separate-and-tab, init_plugin, info)

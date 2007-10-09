/*
 * Extra conversation placement options for Purple
 * Copyright (C) 2004 Gary Kramlich <grim@reaperworld.com>
 *
 * Purple is the legal property of its developers, whose names are too numerous
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301, USA.
 */

#include "../common/pp_internal.h"

#include <conversation.h>
#include <plugin.h>

#include <gtkplugin.h>
#include <gtkconv.h>
#include <gtkconvwin.h>
#include <pidgin.h>

#define SEPANDTAB_PREF "/pidgin/conversations/placement"


static void
conv_placement_sep_ims_tab_chats(PidginConversation *conv) {
	PurpleConversationType type;
	PidginWindow *win = NULL;

	type = purple_conversation_get_type(conv->active_conv);
	win = pidgin_conv_window_last_with_type(type);

	if(type == PURPLE_CONV_TYPE_IM) {
		win = pidgin_conv_window_new();

		pidgin_conv_window_add_gtkconv(win, conv);
		pidgin_conv_window_show(win);
	} else {
		if(!win)
			win = pidgin_conv_window_new();

		pidgin_conv_window_add_gtkconv(win, conv);
		pidgin_conv_window_show(win);
	}
}

static void
conv_placement_sep_chats_tab_ims(PidginConversation *conv) {
	PurpleConversationType type;
	PidginWindow *win = NULL;

	type = purple_conversation_get_type(conv->active_conv);
	win = pidgin_conv_window_last_with_type(type);

	if(type == PURPLE_CONV_TYPE_CHAT) {
		win = pidgin_conv_window_new();

		pidgin_conv_window_add_gtkconv(win, conv);
		pidgin_conv_window_show(win);
	} else {
		if(!win)
			win = pidgin_conv_window_new();

		pidgin_conv_window_add_gtkconv(win, conv);
		pidgin_conv_window_show(win);
	}
}

#if 0
static void
conv_placement_grp_type_sep_prpl(PidginConversation *conv) {
}
#endif

static gboolean
plugin_load(PurplePlugin *plugin) {
	pidgin_conv_placement_add_fnc("sep-ims-tab-chats", _("Separate IM, group Chats"),
							    &conv_placement_sep_ims_tab_chats);
	pidgin_conv_placement_add_fnc("sep-chats-tab-ims", _("Separate Chats, group IMs"),
								&conv_placement_sep_chats_tab_ims);
#if 0
	pidgin_conv_placement_add_fnc("grp-type-sep-prpl",	_("Group by Type, Separate by Protocol"),
								&conv_placement_grp_type_sep_prpl);
#endif
	purple_prefs_trigger_callback(SEPANDTAB_PREF);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin) {
	pidgin_conv_placement_remove_fnc("sep-ims-tab-chats");

	pidgin_conv_placement_remove_fnc("sep-chats-tab-ims");

	purple_prefs_trigger_callback(SEPANDTAB_PREF);

	return TRUE;
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	PIDGIN_PLUGIN_TYPE,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,
	"gtk-plugin_pack-separate_and_tab",
	NULL,
	PP_VERSION,
	NULL,
	NULL,
	"Gary Kramlich <grim@reaperworld.com>",
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
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Separate And Tab");
	info.summary = _("Adds two placement functions for separating and tabbing");
	info.description = _("Adds two new placement functions.\n\n"
						 "One separates IMs and groups chats in tabs\n"
						 "The other separates chats and groups IMs in tabs");

}

PURPLE_INIT_PLUGIN(separate-and-tab, init_plugin, info)

/*
 * smartear.c - SmartEar plugin for libpurple
 * Copyright (c) 2007 John Bailey <rekkanoryo@rekkanoryo.org>
 *
 * Original code copyright (c) 2003-2007 Matt Perry
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
 * 02111-1307, USA
 */

#define PURPLE_PLUGINS

#ifdef HAVE_CONFIG_H
#  include "../pp_config.h"
#endif /* HAVE_CONFIG_H */

#include "../common/pp_internal.h"

/* libpurple headers */
#include <blist.h>
#include <debug.h>
#include <plugin.h>
#include <pluginpref.h>
#include <sound.h>
#include <signals.h>
#include <version.h>

static void
smartear_cb_blistnode_menu_action(PurpleBlistNode *node, gpointer plugin)
{
	/* TODO: Finish me! */
}

static void
smartear_cb_blistnode_menu(PurpleBlistNode *node, GList **menu, gpointer plugin)
{
	PurpleMenuAction *action = NULL;

	/* don't crash when the blistnode is a transient */
	if(purple_blist_node_get_flags(node) & PURPLE_BLIST_NODE_FLAG_NO_SAVE)
		return;

	action = purple_menu_action_new(_("Customize Sounds"),
			PURPLE_CALLBACK(smartear_cb_blistnode_menu_action), plugin, NULL);

	*menu = g_list_prepend(*menu, action);
}

static gboolean
smartear_load(PurplePlugin *plugin)
{
	void *blist_handle = purple_blist_get_handle();
	void *conv_handle = purple_conversations_get_handle();

	/* TODO: make this unset all the pidgin/finch sound prefs 
	 *  - probably can't do this if I plan to stay UI-neutral */

	/* XXX: do we want to "migrate" the pidgin/finch sound prefs by making them
	 * the default for each group if they're turned on? 
	 *  - moot point; see the comment above. */

	/* so we can hook into the blistnode menu and add an option */
	purple_signal_connect(blist_handle, "blist-node-extended-menu", plugin,
			PURPLE_CALLBACK(smartear_cb_blistnode_menu), NULL);

	/* blist signals we need to detect the buddy's activities */
	purple_signal_connect(blist_handle, "buddy-signed-on", plugin,
			PURPLE_CALLBACK(smartear_cb_signonoff), NULL);
	purple_signal_connect(blist_handle, "buddy-signed-off", plugin,
			PURPLE_CALLBACK(smartear_cb_signonoff), NULL);
	purple_signal_connect(blist_handle, "buddy-idle-changed", plugin,
			PURPLE_CALLBACK(smartear_cb_idle), NULL);

	/* conv signals we need to detect activities */
	purple_signal_connect(conv_handle, "received-im-msg", plugin,
			PURPLE_CALLBACK(smartear_cb_received_msg), NULL);
	purple_signal_connect(conv_handle, "sent-im-msg", plugin,
			PURPLE_CALLBACK(smartear_cb_sent_msg), NULL);

	return TRUE;
}

static gboolean
smartear_unload(PurplePlugin *plugin)
{
	/* XXX: since we're going to unset all the pidgin and finch sound prefs,
	 * do we want to keep track of their values and restore them on unload?
	 *  - moot point; see smartear_load above. */

	return TRUE;
}

PurplePluginInfo smartear_info =
{
	PURPLE_PLUGIN_MAGIC,			/* Magic, my ass */
	PURPLE_MAJOR_VERSION,			/* libpurple major version */
	PURPLE_MINOR_VERSION,			/* libpurple minor version */
	PURPLE_PLUGIN_STANDARD,			/* plugin type - this is a normal plugin */
	NULL,							/* UI requirement - we have none */
	0,								/* flags - we have none */
	NULL,							/* dependencies - we have none */
	PURPLE_PRIORITY_DEFAULT,		/* priority - nothing special here */
	"core-plugin_pack-smartear",	/* Plugin ID */
	NULL,							/* name - defined later for i18n */
	PP_VERSION,						/* plugin version - use plugin pack version */
	NULL,							/* summary - defined later for i18n */
	NULL,							/* description - defined later for i18n */
	"John Bailey <rekkanoryo@rekkanoryo.org>",	/* author */
	PP_WEBSITE,						/* plugin website - use plugin pack website */
	gtksmartear_load,				/* plugin load - purple calls this when loading */
	gtksmartear_unload,				/* plugin unload - purple calls this when unloading */
	NULL,							/* plugin destroy - we don't need one */
	NULL,							/* ui_info - we don't need this */
	NULL,							/* extra_info - we don't need this */
	NULL,							/* prefs_info - we don't need this yet */
	NULL,							/* actions - we don't have any */
	NULL,							/* reserved 1 */
	NULL,							/* reserved 2 */
	NULL,							/* reserved 3 */
	NULL							/* reserved 4 */
}

static void
smartear_init(PurplePlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif

	info.name = _("Smart Ear");
	info.summary = _("Assign sounds on a per-buddy or per-group basis");
	info.description = _("Smart Ear allows you to assign sounds on a per-buddy or "
			"per-group basis.  You can configure sounds for sign on, sign off, IM, "
			"and status change events.  Using these features, you can know by the "
			"sounds Pidgin emits which person on your buddy list is doing what.");
}

PURPLE_INIT_PLUGIN(smartear, smartear_init, smartear_info)

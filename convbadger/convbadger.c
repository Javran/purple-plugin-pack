/*
 * GroupMsg - Send an IM to a group of buddies
 * Copyright (C) 2004 Stu Tomlinson <stu@nosnilmot.com>
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
#ifdef HAVE_CONFIG_H
# include "../pp_config.h"
#endif /* HAVE_CONFIG_H */

#define PURPLE_PLUGINS

#include "../common/pp_internal.h"

#include <debug.h>
#include <notify.h>
#include <prpl.h>
#include <request.h>
#include <signals.h>
#include <util.h>
#include <version.h>

static gboolean
plugin_load(PurplePlugin *plugin) {
	void *conv_handle = purple_conversations_get_handle();

	purple_signal_connect(conv_handle, "conversation-created", plugin,
						  PURPLE_CALLBACK(convbadger_conv_created_cb), NULL);
	purple_signal_connect(conv_handle, "deleting-conversation", plugin,
						  PURPLE_CALLBACK(convbadger_conv_destroyed_cb), NULL);

	return TRUE;
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,

	"gtk-plugin_pack-convbadger",
	NULL,
	PP_VERSION,
	NULL,
	NULL,
	"Gary Kramlich <grim@reaperworld.com>",	
	PP_WEBSITE,	

	plugin_load,
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

	info.name = _("Conversation Badger");
	info.summary = _("Badges conversations with the protocol icon.");
	info.description = _("Badges conversations with the protocol icon.");
}

PURPLE_INIT_PLUGIN(convbadger, init_plugin, info)

/*
 * Chronic - Remote sound play triggering
 * Copyright (C) 2006 John Bailey <rekkanoryo@rekkanoryo.org>
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
# include "../gpp_config.h"
#endif /* HAVE_CONFIG_H */

#include "../common/i18n.h"

/* libc */
#include <string.h>

/* GLib */
#include <glib.h>

/* Purple */
#define PURPLE_PLUGINS
#include <conversation.h>
#include <debug.h>
#include <plugin.h>
#include <version.h>

static void
chronic_received_cb(PurpleAccount *account, char *sender, char *message,
		PurpleConversation *conv, PurpleMessageFlags flags)
{
	char *sound = NULL, *path = NULL;

	if(strlen(message) > 3) {
		if(!strncmp(message, "{S ", 3)) {
			sound = (message + 4);
			/* add code to find a matching sound */
			/* purple_sound_play_file(); */
		}
	}

	return;
}

static gboolean
chronic_load(PurplePlugin *plugin)
{
	void *convhandle;
	
	convhandle = purple_conversations_get_handle();

	purple_signal_connect(convhandle, "received-im-msg", plugin,
			PURPLE_CALLBACK(chronic_received_cb), NULL);
	purple_signal_connect(convhandle, "received-chat-msg", plugin,
			PURPLE_CALLBACK(chronic_received_cb), NULL);

	return TRUE;
}

static PurplePluginInfo chronic_info =
{
	PURPLE_PLUGIN_MAGIC,		/* magic?  do you think i'm gullible enough to
							 * believe in magic? */
	PURPLE_MAJOR_VERSION,		/* bet you can't guess what this is */
	PURPLE_MINOR_VERSION,		/* this either */
	PURPLE_PLUGIN_STANDARD,	/* and what about this? */
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,
	"core-plugin_pack-chronic",
	NULL,
	GPP_VERSION,
	NULL,
	NULL,
	"John Bailey <rekkanoryo@rekkanoryo.org>",
	GPP_WEBSITE,
	chronic_load,
	/* comment below is temporary until i decide if i need the function */
	NULL, /*chronic_unload,*/
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static void
chronic_init(PurplePlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	chronic_info.name = _("Chronic");
	chronic_info.summary = _("Sound playing triggers");
	chronic_info.description = _("Allows buddies to remotely trigger sound"
			" playing in your instance of Purple with {S &lt;sound&gt;. Inspired"
			" by #guifications channel resident EvilDennisR and ancient"
			" versions of AOL.  THIS PLUGIN IS NOT YET FUNCTIONAL!"
			"  IT IS USELESS!");
}

PURPLE_INIT_PLUGIN(chronic, chronic_init, chronic_info)

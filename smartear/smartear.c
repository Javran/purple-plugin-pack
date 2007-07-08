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
 *
 * This plugin is a hidden plugin.  It follows preferences set by other plugins
 * which are specific to the libpurple UI in use.  The GTK+ plugin for Pidgin
 * and the GNT plugin for Finch will list this plugin as a dependency, causing
 * libpurple to load this plugin.
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

/* Glib header */
#include <glib.h>

/* Enumerations */

typedef enum {
	SMARTEAR_EVENT_SIGNON,
	SMARTEAR_EVENT_SIGNOFF,
	SMARTEAR_EVENT_IDLECHG,
	SMARTEAR_EVENT_RECEIVEDMSG,
	SMARTEAR_EVENT_SENTMSG
} SmartEarEvent;

/* Helpers */

static const char *
smartear_event_get_setting_string(SmartEarEvent event)
{
	const char *setting = NULL;

	switch(event) {
		case SMARTEAR_EVENT_SIGNON:
			setting = "signon_sound";
			break;
		case SMARTEAR_EVENT_SIGNOFF:
			setting = "signoff_sound";
			break;
		case SMARTEAR_EVENT_IDLECHG:
			setting = "idlechg_sound";
			break;
		case SMARTEAR_EVENT_RECEIVEDMSG:
			setting = "receivedmsg_sound";
			break;
		case SMARTEAR_EVENT_SENTMSG:
			setting = "sentmsg_sound";
			break;
	}

	return setting;
}

static const char *
smartear_sound_get_default(SmartEarEvent event)
{
	/* TODO: Finish me! */
#warning Finish me!!!

	return NULL;
}

static const char *
smartear_sound_determine(const char *bfile, const char *cfile, const char *gfile, SmartEarEvent event)
{
	const char *pfile = NULL;

	/* if the string is "(Default)" then set the pointer to NULL */
	if(!g_ascii_strcmp(bfile, "(Default)"))
		bfile = NULL;
	if(!g_ascii_strcmp(cfile, "(Default)"))
		cfile = NULL;
	if(!g_ascii_strcmp(gfile, "(Default)"))
		gfile = NULL;

	/* determine the sound to play - if the pointer is NULL, try falling back
	 * to another sound - if no sound defined at any level, fall back to the
	 * default */
	if(!bfile)
		if(!cfile)
			if(!gfile)
				pfile = smartear_sound_get_default(event);
			else
				pfile = gfile;
		else
			pfile = cfile;
	else
		pfile = bfile;

	return pfile;
}

static void
smartear_sound_play(PurpleBuddy *buddy, PurpleAccount *account, SmartEarEvent event)
{
	char *bfile = NULL, *cfile = NULL, *gfile = NULL, pfile = NULL, setting = NULL;
	PurpleBlistNode *bnode = (PurpleBlistNode *)buddy,
					*cnode = (PurpleBlistNode *)(bnode->parent),
					*gnode = (PurpleBlistNode *)(cnode->parent);

	/* get the setting string */
	setting = smartear_event_get_setting_string(event);

	/* grab the settings from each blist node in the hierarchy */
	bfile = purple_blist_node_get_string(bnode, setting);
	cfile = purple_blist_node_get_string(cnode, setting);
	gfile = purple_blist_node_get_string(gnode, setting);

	/* determine which sound to play */
	pfile = smartear_sound_determine(bfile, cfile, gfile, event);

	if(pfile)
		purple_sound_play_file(pfile, account);
}

/* Callbacks */

static void
smartear_cb_sent_msg(PurpleAccount *account, const gchar *receiver, const gchar *message)
{
	PurpleBuddy *buddy = purple_find_buddy(account, receiver);

	smartear_sound_play(buddy, account, SMARTEAR_EVENT_SENTMSG);
}

static void
smartear_cb_received_msg(PurpleAccount *account, gchar *sender, char *message,
		PurpleConversation *conv, PurpleMessageFlags flags)
{
	if(!(flags & PURPLE_MESSAGE_SYSTEM)) {
		PurpleBuddy *buddy = purple_find_buddy(account, sender);

		smartear_sound_play(buddy, account, SMARTEAR_EVENT_RECEIVEDMSG);
	}
}

static void
smartear_cb_idle(PurpleBuddy *buddy, gboolean wasidle, gboolean nowidle)
{
	smartear_sound_play(buddy, purple_buddy_get_account(buddy), SMARTEAR_EVENT_IDLECHG);
}

static void
smartear_cb_signoff(PurpleBuddy *buddy)
{
	smartear_sound_play(buddy, purple_buddy_get_account(buddy), SMARTEAR_EVENT_SIGNOFF);
}

static void
smartear_cb_signon(PurpleBuddy *buddy)
{
	smartear_sound_play(buddy, purple_buddy_get_account(buddy), SMARTEAR_EVENT_SIGNON);
}

/* Purple Plugin stuff */

static gboolean
smartear_load(PurplePlugin *plugin)
{
	void *blist_handle = purple_blist_get_handle();
	void *conv_handle = purple_conversations_get_handle();

	/* blist signals we need to detect the buddy's activities */
	purple_signal_connect(blist_handle, "buddy-signed-on", plugin,
			PURPLE_CALLBACK(smartear_cb_signon), NULL);
	purple_signal_connect(blist_handle, "buddy-signed-off", plugin,
			PURPLE_CALLBACK(smartear_cb_signoff), NULL);
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
	return TRUE;
}

PurplePluginInfo smartear_info =
{
	PURPLE_PLUGIN_MAGIC,			/* Magic, my ass */
	PURPLE_MAJOR_VERSION,			/* libpurple major version */
	PURPLE_MINOR_VERSION,			/* libpurple minor version */
	PURPLE_PLUGIN_STANDARD,			/* plugin type - this is a normal plugin */
	NULL,							/* UI requirement - we're invisible! */
	PURPLE_PLUGIN_FLAG_INVISIBLE,	/* flags - we have none */
	NULL,							/* dependencies - we have none */
	PURPLE_PRIORITY_DEFAULT,		/* priority - nothing special here */
	"core-plugin_pack-smartear",	/* Plugin ID */
	NULL,							/* name - defined later for i18n */
	PP_VERSION,						/* plugin version - use plugin pack version */
	NULL,							/* summary - defined later for i18n */
	NULL,							/* description - defined later for i18n */
	"John Bailey <rekkanoryo@rekkanoryo.org>",	/* author */
	PP_WEBSITE,						/* plugin website - use plugin pack website */
	smartear_load,					/* plugin load - purple calls this when loading */
	smartear_unload,				/* plugin unload - purple calls this when unloading */
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

	info.name = _("Smart Ear - Hidden Core Plugin");
	info.summary = _("The Core component of the Smart Ear plugins");
	info.description = _("The Core component of the Smart Ear plugins");
}

PURPLE_INIT_PLUGIN(smartear, smartear_init, smartear_info)

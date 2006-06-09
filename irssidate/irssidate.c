/*
 * irssidate - Let's you know when the day changes
 * Copyright (C) 2005 Gary Kramlich <grim@reaperworld.com>
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
#if HAVE_CONFIG_H
# include "../gpp_config.h"
#endif

#define GAIM_PLUGINS

#define PLUGIN_ID			"core-plugin_pack-irssidate"
#define PLUGIN_NAME			"Irssi Date"
#define PLUGIN_STATIC_NAME	"irssidate"
#define PLUGIN_SUMMARY		"Lets you know when the day changes"
#define PLUGIN_DESCRIPTION	"Lets you know in all open conversations when " \
							"day has changed.  The day changed message is " \
							"not logged."
#define PLUGIN_AUTHOR		"Gary Kramlich <grim@reaperworld.com>"

/* System headers */
#include <glib.h>
#include <time.h>

/* Gaim headers */
#include <conversation.h>
#include <plugin.h>
#include <version.h>

/* Pack/Local headers */
#include "../common/i18n.h"
#include "../common/gpp_compat.h"

static guint timeout_id = 0;
static gint lastday = 0;

static gint
get_day(time_t *t) {
	struct tm *temp = localtime(t);

	return temp->tm_mday;
}

static gint
get_month(time_t *t) {
	struct tm *temp = localtime(t);

	return temp->tm_mon;
}

static gboolean
timeout_cb(gpointer data) {
	time_t t = time(NULL);
	GList *l;
	gint newday = get_day(&t);
	gchar buff[80];
	gchar *message;

	if(newday == lastday)
		return TRUE;

	strftime(buff, sizeof(buff), "%d %b %Y", localtime(&t));
	message = g_strdup_printf(_("Day changed to %s"), buff);

	for(l = gaim_get_conversations(); l; l = l->next) {
		GaimConversation *conv = (GaimConversation *)l->data;

		gaim_conversation_write(conv, NULL, message,
								GAIM_MESSAGE_NO_LOG | GAIM_MESSAGE_SYSTEM,
								t);
		if ((get_day(&t) == 1) && (get_month(&t) == 0)) {
			const gchar *new_year = _("Happy New Year");
			if(conv->type == GAIM_CONV_TYPE_IM)
				gaim_conv_im_send(GAIM_CONV_IM(conv), new_year);
			else if(conv->type == GAIM_CONV_TYPE_CHAT)
				gaim_conv_chat_send(GAIM_CONV_CHAT(conv), new_year);
		}
	}

	g_free(message);

	lastday = newday;

	return TRUE;
}

static gboolean
plugin_load(GaimPlugin *plugin) {
	time_t t = time(NULL);
	lastday = get_day(&t);

	timeout_id = gaim_timeout_add(60000, timeout_cb, NULL);

	return TRUE;
}

static gboolean
plugin_unload(GaimPlugin *plugin) {
	gaim_timeout_remove(timeout_id);

	return TRUE;
}

static GaimPluginInfo info = {
	GAIM_PLUGIN_MAGIC,			/* Magic				*/
	GAIM_MAJOR_VERSION,			/* Gaim Major Version	*/
	GAIM_MINOR_VERSION,			/* Gaim Minor Version	*/
	GAIM_PLUGIN_STANDARD,		/* plugin type			*/
	NULL,						/* ui requirement		*/
	0,							/* flags				*/
	NULL,						/* dependencies			*/
	GAIM_PRIORITY_DEFAULT,		/* priority				*/

	PLUGIN_ID,					/* plugin id			*/
	NULL,						/* name					*/
	GPP_VERSION,				/* version				*/
	NULL,						/* summary				*/
	NULL,						/* description			*/
	PLUGIN_AUTHOR,				/* author				*/
	GPP_WEBSITE,				/* website				*/

	plugin_load,				/* load					*/
	plugin_unload,				/* unload				*/
	NULL,						/* destroy				*/

	NULL,						/* ui_info				*/
	NULL,						/* extra_info			*/
	NULL,						/* prefs_info			*/
	NULL						/* actions				*/
};

static void
init_plugin(GaimPlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GPP_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GPP_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _(PLUGIN_NAME);
	info.summary = _(PLUGIN_SUMMARY);
	info.description = _(PLUGIN_DESCRIPTION);
}

GAIM_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info)

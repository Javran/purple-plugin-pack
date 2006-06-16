/*
 * Conversation Colors
 * Copyright (C) 2006
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

#define PLUGIN_ID			"gtk-plugin_pack-convcolors"
#define PLUGIN_NAME			"Conversation Colors"
#define PLUGIN_STATIC_NAME	"Conversation Colors"
#define PLUGIN_SUMMARY		"Customize colors in the conversation window"
#define PLUGIN_DESCRIPTION	"Customize colors in the conversation window"
#define PLUGIN_AUTHOR		"Sadrul H Chowdhury <sadrul@users.sourceforge.net>"

/* System headers */
#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>

/* Gaim headers */
#include <gtkplugin.h>
#include <version.h>

#include <conversation.h>
#include <gtkconv.h>

/* Pack/Local headers */
#include "../common/i18n.h"

#define	PREF_PREFIX	"/plugins/gtk/" PLUGIN_ID

#define	PREF_SEND	PREF_PREFIX "/send"
#define	PREF_SEND_C	PREF_SEND "/color"
#define	PREF_SEND_F	PREF_SEND "/format"

#define PREF_RECV	PREF_PREFIX "/recv"
#define	PREF_RECV_C	PREF_RECV "/color"
#define	PREF_RECV_F	PREF_RECV "/format"

#define PREF_SYSTEM	PREF_PREFIX "/system"
#define	PREF_SYSTEM_C	PREF_SYSTEM "/color"
#define	PREF_SYSTEM_F	PREF_SYSTEM "/format"

#define PREF_ERROR	PREF_PREFIX "/error"
#define	PREF_ERROR_C	PREF_ERROR "/color"
#define	PREF_ERROR_F	PREF_ERROR "/format"

#define PREF_NICK	PREF_PREFIX "/nick"
#define	PREF_NICK_C	PREF_NICK "/color"
#define	PREF_NICK_F	PREF_NICK "/format"

enum
{
	FONT_BOLD		= 1 << 0,
	FONT_ITALIC		= 1 << 1,
	FONT_UNDERLINE	= 1 << 2
};

#if 0
typedef struct
{
	int bold;
	int italic;
	int underline;
	char color[16];
} GaimMessageFormat;
#endif

/* XXX: Add preference stuff to allow customization */

static gboolean
displaying_msg(GaimAccount *account, const char *who, char **displaying,
				GaimConversation *conv, GaimMessageFlags flags)
{
	struct
	{
		GaimMessageFlags flag;
		const char *prefix;
	} formats[] = 
	{
		{GAIM_MESSAGE_ERROR, PREF_ERROR},
		{GAIM_MESSAGE_NICK, PREF_NICK},
		{GAIM_MESSAGE_SYSTEM, PREF_SYSTEM},
		{GAIM_MESSAGE_SEND, PREF_SEND},
		{GAIM_MESSAGE_RECV, PREF_RECV},
		{0, NULL}
	};
	int i;
	char *tmp;
	gboolean bold, italic, underline;
	int f;
	const char *color;

	for (i = 0; formats[i].prefix; i++)
		if (flags & formats[i].flag)
			break;

	if (!formats[i].prefix)
		return FALSE;

	tmp = g_strdup_printf("%s/color", formats[i].prefix);
	color = gaim_prefs_get_string(tmp);
	g_free(tmp);

	tmp = g_strdup_printf("%s/format", formats[i].prefix);
	f = gaim_prefs_get_int(tmp);
	g_free(tmp);
	bold = (f & FONT_BOLD);
	italic = (f & FONT_ITALIC);
	underline = (f & FONT_UNDERLINE);

	if (color && *color)
	{
		tmp = *displaying;
		*displaying = g_strdup_printf("<FONT COLOR=\"%s\">%s</FONT>", color, tmp);
		g_free(tmp);
	}

	tmp = *displaying;
	*displaying = g_strdup_printf("%s%s%s%s%s%s%s",
						bold ? "<B>" : "</B>",
						italic ? "<I>" : "</I>",
						underline ? "<U>" : "</U>",
						tmp, 
						bold ? "</B>" : "<B>",
						italic ? "</I>" : "<I>",
						underline ? "</U>" : "<U>"
			);
	g_free(tmp);

	return FALSE;
}

static gboolean
plugin_load(GaimPlugin *plugin)
{
	gaim_signal_connect(gaim_gtk_conversations_get_handle(),
					"displaying-im-msg", plugin,
					GAIM_CALLBACK(displaying_msg), NULL);
	gaim_signal_connect(gaim_gtk_conversations_get_handle(),
					"displaying-chat-msg", plugin,
					GAIM_CALLBACK(displaying_msg), NULL);
	return TRUE;
}

static gboolean
plugin_unload(GaimPlugin *plugin)
{
	return TRUE;
}

static GaimPluginInfo info =
{
	GAIM_PLUGIN_MAGIC,			/* Magic				*/
	GAIM_MAJOR_VERSION,			/* Gaim Major Version	*/
	GAIM_MINOR_VERSION,			/* Gaim Minor Version	*/
	GAIM_PLUGIN_STANDARD,		/* plugin type			*/
	GAIM_GTK_PLUGIN_TYPE,		/* ui requirement		*/
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
init_plugin(GaimPlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GPP_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GPP_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _(PLUGIN_NAME);
	info.summary = _(PLUGIN_SUMMARY);
	info.description = _(PLUGIN_DESCRIPTION);

	gaim_prefs_add_none(PREF_PREFIX);

	gaim_prefs_add_none(PREF_SEND);
	gaim_prefs_add_none(PREF_RECV);
	gaim_prefs_add_none(PREF_SYSTEM);
	gaim_prefs_add_none(PREF_ERROR);
	gaim_prefs_add_none(PREF_NICK);

	gaim_prefs_add_string(PREF_SEND_C, "#909090");
	gaim_prefs_add_string(PREF_RECV_C, "#000000");
	gaim_prefs_add_string(PREF_SYSTEM_C, "#50a050");
	gaim_prefs_add_string(PREF_ERROR_C, "#ff0000");
	gaim_prefs_add_string(PREF_NICK_C, "#0000dd");

	gaim_prefs_add_int(PREF_SEND_F, 0);
	gaim_prefs_add_int(PREF_RECV_F, 0);
	gaim_prefs_add_int(PREF_SYSTEM_F, FONT_ITALIC);
	gaim_prefs_add_int(PREF_ERROR_F, FONT_BOLD | FONT_UNDERLINE);
	gaim_prefs_add_int(PREF_NICK_F, FONT_BOLD);
}

GAIM_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info)

/*
 * irssi - Implements several irssi features for Gaim
 * Copyright (C) 2005-2006 Gary Kramlich <grim@reaperworld.com>
 * Copyright (C) 2006 John Bailey <rekkanoryo@rekkanoryo.org>
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

/* all Gaim plugins need to define this */
#define GAIM_PLUGINS

/* define these so the plugin info struct way at the bottom is cleaner */
#define PLUGIN_ID			"gtk-plugin_pack-irssi"
#define PLUGIN_NAME			"Irssi Features"
#define PLUGIN_STATIC_NAME	"irssi"
#define PLUGIN_SUMMARY		"Implements features of the irssi IRC client for " \
							"use in Gaim."
#define PLUGIN_DESCRIPTION	"Implements some features of the IRC client irssi " \
							"to be used in Gaim.  It lets you know in all open " \
							"conversations when the day has changed, adds the " \
							"lastlog command, adds the window command, etc.  " \
							"The day changed message is not logged."
#define PLUGIN_AUTHOR		"\n" \
							"\tGary Kramlich <grim@reaperworld.com>\n" \
							"\tJohn Bailey <rekkanoryo@rekkanoryo.org>\n" \
							"\tSadrul Habib Chowdhury <sadrul@users.sourceforge.net>"

/* System headers */
#include <glib.h>

/* Gaim headers */
#include <gtkplugin.h>
#include <plugin.h>
#include <version.h>

/* irssi headers */
#include "datechange.h"
#include "lastlog.h"
#include "layout.h"
#include "textfmt.h"
#include "window.h"

/* Pack/Local headers */
#include "../common/i18n.h"

static gboolean
irssi_load(GaimPlugin *plugin) {
	irssi_datechange_init(plugin);
	irssi_lastlog_init(plugin);
	irssi_layout_init(plugin);
	irssi_window_init(plugin);
	irssi_textfmt_init(plugin);

	return TRUE;
}

static gboolean
irssi_unload(GaimPlugin *plugin) {
	irssi_textfmt_uninit(plugin);
	irssi_window_uninit(plugin);
	irssi_layout_uninit(plugin);
	irssi_lastlog_uninit(plugin);
	irssi_datechange_uninit(plugin);

	return TRUE;
}

static GaimPluginInfo irssi_info = { /* this tells Gaim about the plugin */
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

	irssi_load,					/* load					*/
	irssi_unload,				/* unload				*/
	NULL,						/* destroy				*/

	NULL,						/* ui_info				*/
	NULL,						/* extra_info			*/
	NULL,						/* prefs_info			*/
	NULL						/* actions				*/
};

static void
irssi_init(GaimPlugin *plugin) {

/* if the user hasn't disabled internationalization support, tell gettext
 * what package we're from and where our translations are, then set gettext
 * to use UTF-8 */
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	/* set these here to allow for translations of the strings */
	irssi_info.name = _(PLUGIN_NAME);
	irssi_info.summary = _(PLUGIN_SUMMARY);
	irssi_info.description = _(PLUGIN_DESCRIPTION);
}

GAIM_INIT_PLUGIN(PLUGIN_STATIC_NAME, irssi_init, irssi_info)


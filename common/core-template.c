/*
 * Plugin Name - Summary
 * Copyright (C) 2004
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
#ifdef HAVE_CONFIG_H
# include "../gpp_config.h"
#endif

#define PURPLE_PLUGINS

#define PLUGIN_ID			"unnamed plugin"
#define PLUGIN_NAME			"unnamed"
#define PLUGIN_STATIC_NAME	"unnamed"
#define PLUGIN_SUMMARY		"summary"
#define PLUGIN_DESCRIPTION	"description"
#define PLUGIN_AUTHOR		"someone <someone@somewhere.tld>"

/* System headers */
#include <glib.h>

/* Purple headers */
#include <plugin.h>
#include <version.h>

/* Pack/Local headers */
#include "../common/i18n.h"

static gboolean
plugin_load(PurplePlugin *plugin) {
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin) {
	return TRUE;
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,			/* Magic				*/
	PURPLE_MAJOR_VERSION,			/* Purple Major Version	*/
	PURPLE_MINOR_VERSION,			/* Purple Minor Version	*/
	PURPLE_PLUGIN_STANDARD,		/* plugin type			*/
	NULL,						/* ui requirement		*/
	0,							/* flags				*/
	NULL,						/* dependencies			*/
	PURPLE_PRIORITY_DEFAULT,		/* priority				*/

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
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _(PLUGIN_NAME);
	info.summary = _(PLUGIN_SUMMARY);
	info.description = _(PLUGIN_DESCRIPTION);
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info)

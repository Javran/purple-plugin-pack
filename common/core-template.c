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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02111-1301, USA.
 */

#include "../common/pp_internal.h"

#define PLUGIN_ID			"unnamed plugin"
#define PLUGIN_STATIC_NAME	"unnamed"
#define PLUGIN_AUTHOR		"someone <someone@somewhere.tld>"

/* Purple headers */
#include <plugin.h>

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
	PURPLE_PLUGIN_STANDARD,			/* plugin type			*/
	NULL,							/* ui requirement		*/
	0,								/* flags				*/
	NULL,							/* dependencies			*/
	PURPLE_PRIORITY_DEFAULT,		/* priority				*/

	PLUGIN_ID,						/* plugin id			*/
	NULL,							/* name					*/
	PP_VERSION,						/* version				*/
	NULL,							/* summary				*/
	NULL,							/* description			*/
	PLUGIN_AUTHOR,					/* author				*/
	PP_WEBSITE,						/* website				*/

	plugin_load,					/* load					*/
	plugin_unload,					/* unload				*/
	NULL,							/* destroy				*/

	NULL,							/* ui_info				*/
	NULL,							/* extra_info			*/
	NULL,							/* prefs_info			*/
	NULL,							/* actions				*/
	NULL,							/* reserved 1			*/
	NULL,							/* reserved 2			*/
	NULL,							/* reserved 3			*/
	NULL							/* reserved 4			*/
};

static void
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("unnamed");
	info.summary = _("summary");
	info.description = _("description");
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info)

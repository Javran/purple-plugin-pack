/*
 * irssi - Implements several irssi features for Purple
 * Copyright (C) 2005-2008 Gary Kramlich <grim@reaperworld.com>
 * Copyright (C) 2006-2008 John Bailey <rekkanoryo@rekkanoryo.org>
 * Copyright (C) 2006-2008 Sadrul Habib Chowdhury <sadrul@users.sourceforge.net>
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

/* If you can't figure out what this line is for, DON'T TOUCH IT. */
#include "../common/pp_internal.h"

/* Pidgin headers */
#include <gtkplugin.h>

/* Local plugin headers */
#include "irssi.h"

static gboolean
irssi_load(PurplePlugin *plugin) {
	irssi_datechange_init(plugin);
	irssi_lastlog_init(plugin);
	irssi_layout_init(plugin);
	irssi_window_init(plugin);
	irssi_textfmt_init(plugin);

	return TRUE;
}

static gboolean
irssi_unload(PurplePlugin *plugin) {
	irssi_textfmt_uninit(plugin);
	irssi_window_uninit(plugin);
	irssi_layout_uninit(plugin);
	irssi_lastlog_uninit(plugin);
	irssi_datechange_uninit(plugin);

	return TRUE;
}

static PurplePluginPrefFrame *
irssi_pref_frame(PurplePlugin *plugin) {
	PurplePluginPrefFrame *frame;
	PurplePluginPref *pref;

	frame = purple_plugin_pref_frame_new();

	pref = purple_plugin_pref_new_with_label(_("Enable Features:"));
	purple_plugin_pref_frame_add(frame, pref);

	pref = purple_plugin_pref_new_with_name_and_label(TEXTFMT_PREF, _("Text Formatting"));
	purple_plugin_pref_frame_add(frame, pref);

	pref = purple_plugin_pref_new_with_name_and_label(DATECHANGE_PREF, _("Date Change Notification"));
	purple_plugin_pref_frame_add(frame, pref);

	pref = purple_plugin_pref_new_with_name_and_label(SENDNEWYEAR_PREF, _("Happy New Year Message"));
	purple_plugin_pref_frame_add(frame, pref);

	return frame;
}

static PurplePluginUiInfo irssi_prefs_info = {
	irssi_pref_frame,
	0,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginInfo irssi_info = { /* this tells Purple about the plugin */
	PURPLE_PLUGIN_MAGIC,			/* Magic				*/
	PURPLE_MAJOR_VERSION,			/* Purple Major Version	*/
	PURPLE_MINOR_VERSION,			/* Purple Minor Version	*/
	PURPLE_PLUGIN_STANDARD,			/* plugin type			*/
	PIDGIN_PLUGIN_TYPE,				/* ui requirement		*/
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

	irssi_load,						/* load					*/
	irssi_unload,					/* unload				*/
	NULL,							/* destroy				*/

	NULL,							/* ui_info				*/
	NULL,							/* extra_info			*/
	&irssi_prefs_info,				/* prefs_info			*/
	NULL,							/* actions				*/
	NULL,							/* reserved 1			*/
	NULL,							/* reserved 2			*/
	NULL,							/* reserved 3			*/
	NULL							/* reserved 4			*/
};

static void
irssi_init(PurplePlugin *plugin) {

/* if the user hasn't disabled internationalization support, tell gettext
 * what package we're from and where our translations are, then set gettext
 * to use UTF-8 */
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	/* set these here to allow for translations of the strings */
	irssi_info.name = _("Irssi Features");
	irssi_info.summary = _("Implements features of the irssi IRC client for "
			"use in Pidgin.");
	irssi_info.description = _("Implements some features of the IRC client "
			"irssi to be used in Purple.  It lets you know in all open "
			"conversations when the day has changed, adds the lastlog command, "
			"adds the window command, etc.  The day changed message is not logged.");

	purple_prefs_add_none(PREFS_ROOT_PARENT);
	purple_prefs_add_none(PREFS_ROOT);
	purple_prefs_add_bool(TEXTFMT_PREF, TRUE);
	purple_prefs_add_bool(DATECHANGE_PREF, TRUE);
	purple_prefs_add_bool(SENDNEWYEAR_PREF, TRUE);
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, irssi_init, irssi_info)


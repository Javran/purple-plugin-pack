/*
 * Album (Buddy Icon Archiver)
 *
 * Copyright (C) 2005-2006, Sadrul Habib Chowdhury <imadil@gmail.com>
 * Copyright (C) 2005-2006, Richard Laager <rlaager@users.sf.net>
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

#include <errno.h>
#include <string.h>

#include <glib.h>

/* We want to use the gstdio functions when possible so that non-ASCII
 * filenames are handled properly on Windows. */
#if GLIB_CHECK_VERSION(2,6,0)
#include <glib/gstdio.h>
#else
#include <sys/stat.h>
#define g_fopen fopen
#define g_stat stat
#endif

#ifndef  _WIN32
#include <sys/types.h>
#include <utime.h>
#include <unistd.h>
#endif

#ifndef PURPLE_PLUGINS
#define PURPLE_PLUGINS
#endif

#include <buddyicon.h>
#include <debug.h>
#include <plugin.h>
#include <util.h>
#include <version.h>

#include <cipher.h>

#include "album.h"

/* GUI */
#include <gtkplugin.h>
#include "album-ui.h"

GHashTable *buddy_windows;

#define PLUGIN_NAME        "Album"
#define PLUGIN_SUMMARY     "Archives buddy icons."
#define PLUGIN_DESCRIPTION "Enable this plugin to automatically archive all buddy icons."
#define PLUGIN_AUTHOR      "Richard Laager <rlaager@guifications.org>" \
                           "\n\t\t\tSadrul Habib Chowdhury <imadil@gmail.com>"

/*****************************************************************************
 * Prototypes                                                                *
 *****************************************************************************/

static gboolean plugin_load(PurplePlugin *plugin);


/*****************************************************************************
 * Plugin Info                                                               *
 *****************************************************************************/

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	0,

	PURPLE_PLUGIN_STANDARD,            /**< type           */
	PIDGIN_PLUGIN_TYPE,                /**< ui_requirement */
	0,                                 /**< flags          */
	NULL,                              /**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,           /**< priority       */
	PLUGIN_ID,                         /**< id             */
	NULL,                              /**< name           */
	PP_VERSION,                        /**< version        */
	PLUGIN_SUMMARY,                    /**< summary        */
	NULL,                              /**< description    */
	PLUGIN_AUTHOR,                     /**< author         */
	PP_WEBSITE,                        /**< homepage       */
	plugin_load,                       /**< load           */
	NULL,                              /**< unload         */
	NULL,                              /**< destroy        */
	NULL,                              /**< ui_info        */
	NULL,                              /**< extra_info     */
	NULL,                              /**< prefs_info     */
	album_get_plugin_actions
};


/*****************************************************************************
 * Helpers                                                                   *
 *****************************************************************************/

char *album_buddy_icon_get_dir(PurpleAccount *account, const char *name)
{
	PurplePlugin *prpl;
	const char *prpl_name;
	char *acct_name;
	char *buddy_name;
	char *dir;

	g_return_val_if_fail(account != NULL, NULL);
	g_return_val_if_fail(name != NULL, NULL);

	/* BUILD THE DIRECTORY PATH */
	prpl = purple_find_prpl(purple_account_get_protocol_id(account));
	if (!prpl)
		g_return_val_if_reached(NULL);

	prpl_name = PURPLE_PLUGIN_PROTOCOL_INFO(prpl)->list_icon(account, NULL);

	acct_name = g_strdup(purple_escape_filename(purple_normalize(account,
				purple_account_get_username(account))));

	buddy_name = g_strdup(purple_escape_filename(purple_normalize(account, name)));

	dir = g_build_filename(purple_buddy_icons_get_cache_dir(), prpl_name, acct_name, buddy_name, NULL);
	g_free(acct_name);
	g_free(buddy_name);

	return dir;
}


/*****************************************************************************
 * Callbacks                                                                 *
 *****************************************************************************/

static void buddy_icon_cached_cb(PurpleBuddyIcon *icon, PurpleBuddy *buddy,
                                 const char *filename, const char *old_icon, gpointer data)
{
	char *dir;

	const void *icon_data;
	size_t len;

#if PURPLE_MAJOR_VERSION > 1
	PurpleCipherContext *context;
	gchar digest[41];
	gboolean got_digest;
#else
	unsigned char digest[20];
#endif

	char *checksum;
#if PURPLE_MAJOR_VERSION > 1
	const char *ext;
#else
	const char *ext = "icon";
#endif
	char *new_filename;
	char *path;

#ifndef _WIN32
	int status;
#endif
	FILE *file = NULL;


	/* BUILD THE DIRECTORY PATH & CREATE DIRECTORY */
	dir = album_buddy_icon_get_dir(purple_buddy_get_account(buddy), purple_buddy_get_name(buddy));
	purple_build_dir (dir, S_IRUSR | S_IWUSR | S_IXUSR);


	/* GET THE DATA */
	icon_data = purple_buddy_icon_get_data(icon, &len);


	/* HASH THE DATA */
#if PURPLE_MAJOR_VERSION > 1
	context = purple_cipher_context_new_by_name("sha1", NULL);
	if (context == NULL) {
		purple_debug_info(PLUGIN_STATIC_NAME, "could not find sha1 cipher\n");
		return;
	}

	purple_cipher_context_append(context, icon_data, len);

	got_digest = purple_cipher_context_digest_to_str(context,
			sizeof(digest), digest, NULL);

	if (got_digest)
		checksum = g_strdup(digest);
	else
		checksum = g_strdup_printf("%x", g_random_int());

	purple_cipher_context_destroy(context);
#else
	shaBlock((unsigned char *)icon_data, len, digest);
	checksum = g_strdup_printf("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
	                           "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
	                           digest[0],  digest[1],  digest[2],  digest[3],
	                           digest[4],  digest[5],  digest[6],  digest[7],
	                           digest[8],  digest[9],  digest[10], digest[11],
	                           digest[12], digest[13], digest[14], digest[15],
	                           digest[16], digest[17], digest[18], digest[19]);
#endif

	/* BUILD THE FILENAME */
#if PURPLE_MAJOR_VERSION > 1
	/* This was backported to Purple 1.4.0 as well, but I'd
	 * rather not use it, as that'd introduce a dependency
	 * on Purple 1.4.0. */
	ext = purple_buddy_icon_get_type(icon);
	if (ext == NULL)
		ext = "icon";
#else
	if (len >= 4)
	{
		if (!strncmp(icon_data, "BM", 2))
			ext = "bmp";
		else if (!strncmp(icon_data, "GIF8", 4))
			ext = "gif";
		else if (!strncmp(icon_data, "\xff\xd8\xff\xe0", 4))
			ext = "jpg";
		else if (!strncmp(icon_data, "\x89PNG", 4))
			ext = "png";
	}
#endif

	new_filename = g_strdup_printf("%s.%s", checksum, ext);
	g_free(checksum);
	path = g_build_filename(dir, new_filename, NULL);
	g_free(dir);
	g_free(new_filename);


#ifndef _WIN32
	/* TRY MAKING A HARD LINK */
	status = link(filename, path);
	if (status != 0)
	{
		if (status != EEXIST)
		{
#endif
			/* WRITE THE DATA */
			if ((file = g_fopen(path, "wb")) != NULL)
			{
				fwrite(icon_data, 1, len, file);
				fclose(file);
			}
#ifndef _WIN32
		}
		else
		{
			utime(path, NULL);
		}
	}
#endif
}

static void cache_for_buddy(gpointer key, PurpleBuddy *buddy, gpointer data)
{
	PurpleBuddyIcon *icon;
	const char *filename;
#if PURPLE_MAJOR_VERSION > 1
	char *full_path;
#else
	struct stat st;
#endif

	icon = purple_buddy_get_icon(buddy);
	if (icon == NULL)
		return;

	filename = purple_blist_node_get_string((PurpleBlistNode*)buddy, "buddy_icon");
	if (filename == NULL)
		return;

	purple_debug_misc(PLUGIN_STATIC_NAME, "Caching existing icon for buddy: %s\n",
	                             purple_buddy_get_name(buddy));

#if PURPLE_MAJOR_VERSION > 1
	full_path = purple_buddy_icons_get_full_path(filename);

	/* Pretend this was just cached by Purple. */
	buddy_icon_cached_cb(icon, buddy, full_path, NULL, NULL);

	g_free(full_path);
#else
	if (g_stat(filename, &st) == 0)
	{
		/* Pretend this was just cached by Purple. */
		buddy_icon_cached_cb(icon, buddy, filename, NULL, NULL);
	}
	else
	{
		char *tmp = g_build_filename(purple_buddy_icons_get_cache_dir(), filename, NULL);

		/* Pretend this was just cached by Purple. */
		buddy_icon_cached_cb(icon, buddy, tmp, NULL, NULL);

		g_free(tmp);
	}
#endif
}

static
#if PURPLE_MAJOR_VERSION > 1
void
#else
gboolean
#endif
cache_existing_icons(gpointer data)
{
	/* Cache Existing Icons */
	g_hash_table_foreach(purple_get_blist()->buddies, (GHFunc)cache_for_buddy, data);

#if PURPLE_MAJOR_VERSION < 2
	return TRUE;
#endif
}

#if PURPLE_MAJOR_VERSION > 1
static void buddy_icon_changed_cb(PurpleBuddy *buddy)
{
	/* This callback doesn't use either of the arguments besides buddy,
	 * so it's convenient to reuse it here. */
	cache_for_buddy(NULL, buddy, NULL);
}
#endif


/*****************************************************************************
 * Plugin Code                                                               *
 *****************************************************************************/

static gboolean plugin_load(PurplePlugin *plugin)
{
#if PURPLE_MAJOR_VERSION < 2
	guint handle;

	handle = purple_signal_connect(purple_buddy_icons_get_handle(), "buddy-icon-cached",
	                             plugin, PURPLE_CALLBACK(buddy_icon_cached_cb), NULL);

	if (handle == 0)
	{
		/* This version of Purple doesn't have the signal. Fake it with a timeout function. */

		/* Cache icons every ten minutes. */
		g_timeout_add(600, cache_existing_icons, NULL);
	}
#else
	purple_signal_connect(purple_blist_get_handle(), "buddy-icon-changed",
	                    plugin, PURPLE_CALLBACK(buddy_icon_changed_cb), NULL);
#endif

	/* Some UI stuff here. */
	purple_signal_connect_priority(
#if PURPLE_MAJOR_VERSION < 2
	                             purple_buddy_icons_get_handle(),
	                             "buddy-icon-cached",
#else
	                             purple_blist_get_handle(),
				     "buddy-icon-changed",
#endif
	                             plugin,
	                             PURPLE_CALLBACK(album_update_runtime),
	                             NULL,
	                             PURPLE_SIGNAL_PRIORITY_DEFAULT + 1);

	purple_signal_connect(purple_blist_get_handle(),
	                    "blist-node-extended-menu",
	                    plugin,
	                    PURPLE_CALLBACK(album_blist_node_menu_cb),
	                    NULL);

	cache_existing_icons(NULL);

	buddy_windows = g_hash_table_new_full(icon_viewer_hash, icon_viewer_equal, g_free, g_free);

	return TRUE;
}

static void plugin_init(PurplePlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _(PLUGIN_NAME);
	info.summary = _(PLUGIN_SUMMARY);
	info.description = _(PLUGIN_DESCRIPTION);

	/* Setup preferences. */
	purple_prefs_add_none(PREF_PREFIX);
	purple_prefs_add_int(PREF_WINDOW_HEIGHT, 258);
	purple_prefs_add_int(PREF_WINDOW_WIDTH, 362);
	purple_prefs_add_int(PREF_ICON_SIZE, 1);
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, plugin_init, info)

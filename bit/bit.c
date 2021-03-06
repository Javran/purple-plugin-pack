/*
 * Copyright (C) 2005-2008 Peter Lawler <bleeter from users.sf.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301, USA.
 */

/* If you can't figure out what this line is for, DON'T TOUCH IT. */
#include "../common/pp_internal.h"

#include <buddyicon.h>
#include <debug.h>
#include <notify.h>
#include <plugin.h>
#include <request.h>

PurplePlugin *bit = NULL; /* the request api prefers this for a plugin */
static PurpleBuddyList *buddies = NULL;

/* TODO: Add a function to clear unused icons */
/* TODO: Ensure all this stuff I have at the moment is safe for others to use */

static void
blist_iterate_action(gboolean remove)
{
	PurpleBlistNode *node = NULL;
	PurpleConversation *conv = NULL;
	gint n;

	/* this grabs the purple buddy list, which will be walked through */
	buddies = purple_get_blist();

	/* Use the utility function to loop over the nodes of the tree */
	for (node = buddies->root; node && PURPLE_BLIST_NODE_IS_BUDDY(node); 
		 node = purple_blist_node_next(node, TRUE)) {
		PurpleBuddy *buddy = (PurpleBuddy *)node;
		const char *tmpname = purple_buddy_get_name(buddy);
		PurpleBuddyIcon *icon = purple_buddy_get_icon(buddy);
		if (icon != NULL) {
#if 0
			purple_debug_info("bit", "Processing %s (%p)\n", tmpname, icon);
			if (!icon->ref_count > 0 && remove == TRUE) {
				for ( n = icon->ref_count; n !=0; n-- ) {
					purple_debug_info("bit", "ref_count: %d\n", n);
					purple_buddy_icon_unref(icon);
				}
			}
#endif
			/* XXX: This *may* cause a segfault. - Sadrul */
			if (remove == TRUE) {
				purple_debug_info("bit", "Uncaching icon for %s\n", tmpname);
#if 0
				/* XXX: The new buddy icon API doesn't have purple_buddy_icon_uncache() */
				purple_buddy_icon_uncache(buddy);
#endif
				/* XXX: This *definately* causes a segfault. From reading the 
				 * source, I may not need to unref but just straight destroy it
				 * haven't played/investigated enough to decide if I want to
				 * keep/move/delete this - Bleeter

				purple_debug_info("bit", "Destroying icon for %s\n", tmpname);
				purple_buddy_icon_destroy(icon);*/
			}
		} else {
			if (remove == TRUE)
				purple_debug_info("bit", "No icon to flush for %s\n", tmpname);
		}

		if (purple_account_is_connected(purple_buddy_get_account(buddy))) {
			purple_debug_info("bit", "Updating icon for %s\n",
							 tmpname);
			purple_blist_update_buddy_icon(buddy);
			conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM,
									  tmpname,purple_buddy_get_account(buddy));
			if (conv != NULL)
				purple_conversation_update(conv, PURPLE_CONV_UPDATE_ICON);
		}
	}
}

static void
flush_buddy_icon_action(PurplePluginAction *action)
{
	blist_iterate_action(TRUE);
}

static void
refresh_buddy_icon_action(PurplePluginAction *action)
{
	blist_iterate_action(FALSE);
}

#if 0
static void
destroy_unused_icons_action(PurplePluginAction *action)
{
	GDir *dir;
	GList *l;
	const char *path, *filename;
	const char *type;

	path = purple_buddy_icons_get_cache_dir();
	if (path == NULL) {
		gchar *str;
		str = g_strdup_printf(_("Unable to locate the buddy icon cache directory %s"), path);
		purple_debug_error("bit", str);
		purple_notify_error(bit, _("Destroy Unused Icons"), _("Unable to locate"),
						  str);
		return;
	}

	if (!(dir = g_dir_open(path, 0, NULL))) {
		gchar *str;
		str = g_strdup_printf(_("Unable to read the buddy icon cache directory %s"), path);
		purple_debug_error("bit", str);
		purple_notify_error(bit, _("Destroy Unused Icons"), _("Unable to read"),
						  str);
		return;
	}

	while ((filename = g_dir_read_name(dir))) {

		PurpleBlistNode *cur_node = NULL;
		PurpleConversation *conv = NULL;
		gint n;

		buddies = purple_get_blist();
		for (cur_node = buddies->root; cur_node; 
			 cur_node = purple_blist_node_next(cur_node, TRUE)) {
			if(PURPLE_BLIST_NODE_IS_BUDDY(cur_node)) {
				PurpleBuddy *buddy = (PurpleBuddy *)cur_node;
				const char *tmpname = purple_buddy_get_name(buddy);
				PurpleBuddyIcon *icon = purple_buddy_get_icon(buddy);
				if (icon != NULL) {
					/* store each found icon FILENAME into *l */
					/* checksums are done in prpl, so don't bother trying */
				}
			}
		}
		/* remove files not in SOMWHERE*/
		purple_debug_info("bit", "Filename %s\n", filename);
		type = purple_buddy_icon_get_type(filename);
		purple_debug_info("bit", "Type %s\n", type);
	}
}
#endif

static GList *
bit_actions(PurplePlugin *plugin, gpointer context)
{
	GList *list = NULL;
	PurplePluginAction *act = NULL;

#if 0
/* buddy icon structs currently suck, I think
   it's impossible to tell from a filename which buddy it's associated with
   without going through every file, and the blist...
   ... a huge hash type table *may help*, but I'd consider it highly inefficient
   then again, some of the stuff in here ain't exactly a TGV either */

	act = purple_plugin_action_new(_("Destroy Unused Icons"),
			destroy_unused_icons_action);
	list = g_list_append(list, act);
#endif
	act = purple_plugin_action_new(_("Flush Buddy Icons"),
			flush_buddy_icon_action);
	list = g_list_append(list, act);

	act = purple_plugin_action_new(_("Refresh Buddy Icons"),
			refresh_buddy_icon_action);
	list = g_list_append(list, act);

	purple_debug_info("bit", "Action list created\n");

	return list;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,								/**< magic			*/
	PURPLE_MAJOR_VERSION,								/**< major version	*/
	PURPLE_MINOR_VERSION,								/**< minor version	*/
	PURPLE_PLUGIN_STANDARD,								/**< type			*/
	NULL,												/**< ui_requirement	*/
	0,													/**< flags			*/
	NULL,												/**< dependencies	*/
	PURPLE_PRIORITY_DEFAULT,							/**< priority		*/

	"core-plugin_pack-bit",								/**< id				*/
	NULL,												/**< name			*/
	PP_VERSION,											/**< version		*/
	NULL,												/**  summary		*/
	NULL,												/**  description	*/
	"Peter Lawler <bleeter from users.sf.net>",
														/**< authors		*/
	PP_WEBSITE,											/**< homepage		*/

	NULL,												/**< load			*/
	NULL,												/**< unload			*/
	NULL,												/**< destroy		*/

	NULL,												/**< ui_info		*/
	NULL,												/**< extra_info		*/
	NULL,												/**< prefs_info		*/
	bit_actions,										/**< actions		*/
	NULL,												/**< reserved 1		*/
	NULL,												/**< reserved 2		*/
	NULL,												/**< reserved 3		*/
	NULL												/**< reserved 4		*/
};

static void
init_plugin(PurplePlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif
	info.name = _("Buddy Icon Tools");
	info.summary = _("Tools to manipulate buddy icons. *DANGEROUS*");
	info.description = _("Whilst working on Purple 2.0.0, I found a need to "
			"destroy all my buddies' buddy icons.  There's nothing to do "
			"these functions in Purple, so here they are. Completely, "
			"thoroughly untested.");

	bit = plugin; /* handle needed for request API file selector */
}

PURPLE_INIT_PLUGIN(bit, init_plugin, info)


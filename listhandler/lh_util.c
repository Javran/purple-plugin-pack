/*
 * Purple Plugin Pack
 * Copyright (C) 2003-2008
 * See ../AUTHORS for a list of all authors
 *
 * listhandler: Provides importing, exporting, and copying functions
 * 				for accounts' buddy lists.
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

#include "listhandler.h"

void
lh_util_add_buddy(const gchar *group, PurpleGroup *purple_group,
			const gchar *buddy, const gchar *alias, PurpleAccount *account,
			const gchar *buddynotes, gint signed_on, gint signed_off, gint lastseen,
			gint last_seen, const gchar *gf_theme, const gchar *icon_file, gchar *lastsaid)
{
	PurpleBuddy *purple_buddy = NULL;
	PurpleBlistNode *node = NULL;
	
	purple_buddy = purple_buddy_new(account, buddy, alias);
	node = (PurpleBlistNode *)purple_buddy;

	purple_blist_add_buddy(purple_buddy, NULL, purple_group, NULL);
	purple_account_add_buddy(account, purple_buddy);

	if(buddynotes)
		purple_blist_node_set_string(node, "notes", buddynotes);
	if(signed_on)
		purple_blist_node_set_int(node, "signedon", signed_on);
	if(signed_off)
		purple_blist_node_set_int(node, "signedoff", signed_off);
	if(lastseen)
		purple_blist_node_set_int(node, "lastseen", lastseen);
	if(last_seen)
		purple_blist_node_set_int(node, "last_seen", last_seen);
	if(gf_theme)
		purple_blist_node_set_string(node, "guifications-theme", gf_theme);
	if(icon_file)
		purple_blist_node_set_string(node, "buddy_icon", icon_file);
	if(lastsaid)
		purple_blist_node_set_string(node, "lastsaid", lastsaid);

	purple_debug_info("listhandler: import",
			"group: %s\tbuddy: %s\talias: %s\thas been added to the list\n",
			group, buddy, alias ? alias : "NULL");

	return;
}

void
lh_util_add_to_blist(GList *buddies, GList *groups)
{
	GList *tmpb = buddies, *tmpg = groups;

	/* walk through both GLists  */
	while(tmpb && tmpb->data && tmpg && tmpg->data) {
		/* add the current buddy to the correct group in the Purple blist */
		purple_blist_add_buddy((PurpleBuddy *)(tmpb->data), NULL,
								(PurpleGroup *)(tmpg->data), NULL);

		purple_debug_info("listhandler: import", "added a buddy to purple blist\n");

		/* go to the next element in each list */
		tmpb = g_list_next(tmpb);
		tmpg = g_list_next(tmpg);
	}

	return;
}


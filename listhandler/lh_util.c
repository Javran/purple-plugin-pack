/*
 * Purple Plugin Pack
 * Copyright (C) 2003-2005
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "listhandler.h"

void
lh_util_add_buddy(const gchar *group, PurpleGroup *purple_group,
			const gchar *buddy, const gchar *alias, PurpleAccount *account,
			const gchar *buddynotes)
{
	/* create a PurpleBuddy and add it to the list in the specified group.
	 * The first NULL is because we have no contact.  Let the user do that.
	 * The second NULL is because we're prepending to the group.  Let the
	 * user organize in whatever order he/she wants. This also is surprisingly
	 * easy to do. */
	PurpleBuddy *purple_buddy = purple_buddy_new(account, buddy, alias);
	purple_blist_add_buddy(purple_buddy, NULL, purple_group, NULL);
	purple_account_add_buddy(account, purple_buddy);

	if(buddynotes)
		purple_blist_node_set_string((PurpleBlistNode *)purple_buddy, "notes", buddynotes);

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

		purple_debug_info("listhandler: import", "added a buddy to pidgin "
						  "blist\n");

		/* go to the next element in each list */
		tmpb = g_list_next(tmpb);
		tmpg = g_list_next(tmpg);
	}

	return;
}


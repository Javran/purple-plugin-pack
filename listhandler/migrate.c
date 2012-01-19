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
#include "migrate.h"

static GList *buddies = NULL, *groups = NULL;
static PurpleAccount *source_account = NULL, *target_account = NULL;
static const gchar *valid_target_prpl_id = NULL;

static gboolean
lh_migrate_filter(PurpleAccount *account)
{
	/* get the protocol plugin id for the account passsed in */
	const gchar *prpl_id = purple_account_get_protocol_id(account);

	/* if prpl_id is NULL, the account isn't valid */
	if(!prpl_id)
		return FALSE;

	/* if prpl_id matches the prpl id for valid target accounts,
	 * show this account in the request list */
	if(!strcmp(prpl_id, valid_target_prpl_id))
		return TRUE;

	return FALSE;
}

static void
lh_migrate_build_lists(void)
{
	PurpleBuddyList *blist = NULL;
	PurpleBlistNode *root = NULL, *g = NULL, *c = NULL, *b = NULL;

	blist = purple_get_blist();
	root = blist->root;

	/* walk the blist tree and build a list of the buddies and a list of
	 * the groups corresponding to each buddy */
	/* group level */
	for(g = root; g && PURPLE_BLIST_NODE_IS_GROUP(g); g = g->next)
		/* contact level */
		for(c = g->child; c && PURPLE_BLIST_NODE_IS_CONTACT(c); c = c->next)
			/* buddy level */
			for(b = c->child; b && PURPLE_BLIST_NODE_IS_BUDDY(b); b = b->next) {
				PurpleGroup *tmp_group = purple_group_new(((PurpleGroup *)g)->name);
				PurpleBuddy *tmp_buddy = (PurpleBuddy *)b;

				/* if this buddy is on the source account then add it
				 * to the GLists */
				if(purple_buddy_get_account(tmp_buddy) == source_account) {
					PurpleBuddy *tmp_buddy_2 = purple_buddy_new(target_account,
								  				purple_buddy_get_name(tmp_buddy),
												purple_buddy_get_alias(tmp_buddy));

					groups = g_list_prepend(groups, tmp_group);
					buddies = g_list_prepend(buddies, tmp_buddy_2);
				}
			}

	return;
}

static void
lh_migrate_target_request_cb(void *ignored, PurpleRequestFields *fields)
{
	/* grab the target account from the request field */
	target_account = purple_request_fields_get_account(fields,
													"migrate_target_acct");

	/* now build GLists of the buddies and corresponding groups */
	lh_migrate_build_lists();

	/* add the buddies to the Purple buddy list */
	lh_util_add_to_blist(buddies, groups);

	/* add the buddies to the server-side list */
#if PURPLE_VERSION_CHECK(3,0,0)
	purple_account_add_buddies(target_account, buddies, NULL);
#else
	purple_account_add_buddies(target_account, buddies);
#endif

	/* now free the lists that were created */
	g_list_free(buddies);
	g_list_free(groups);

	return;
}

static void
lh_migrate_source_request_cb(void *ignored, PurpleRequestFields *fields)
{
	PurpleRequestFields *request;
	PurpleRequestFieldGroup *group;
	PurpleRequestField *field;

	source_account = purple_request_fields_get_account(fields,
													"migrate_source_acct");
	valid_target_prpl_id = purple_account_get_protocol_id(source_account);

	/* It seems Purple is super-picky about the order of these first three calls */
	/* create a request */
	request = purple_request_fields_new();

	/* now create a field group */
	group = purple_request_field_group_new(NULL);
	/* and add that group to the request created above */
	purple_request_fields_add_group(request, group);

	/* create a field */
	field = purple_request_field_account_new("migrate_target_acct",
										_("Account"), NULL);

	/* mark the field as required */
	purple_request_field_set_required(field, TRUE);

	/* filter this list so that only accounts with the same prpl as the
	 * source account can be chosen as a destination */
	purple_request_field_account_set_filter(field, lh_migrate_filter);

	/* add the field to the group created above */
	purple_request_field_group_add_field(group, field);

	/* and finally we can create the request */
	purple_request_fields(purple_get_blist(), _("Listhandler - Copying"),
						_("Choose the account to add buddies to:"), NULL,
						request, _("_Add"),
						G_CALLBACK(lh_migrate_target_request_cb), _("_Cancel"),
						NULL, NULL, NULL, NULL, NULL);

	return;
}

void
lh_migrate_action_cb(PurplePluginAction *action)
{
	PurpleRequestFields *request;
	PurpleRequestFieldGroup *group;
	PurpleRequestField *field;

	/* It seems Purple is super-picky about the order of these first three calls */
	/* create a request */
	request = purple_request_fields_new();

	/* now create a field group */
	group = purple_request_field_group_new(NULL);
	/* and add that group to the request created above */
	purple_request_fields_add_group(request, group);

	/* create a field */
	field = purple_request_field_account_new("migrate_source_acct",
										_("Account"), NULL);

	/* mark the field as required */
	purple_request_field_set_required(field, TRUE);

	/* let's show offline accounts too */
	purple_request_field_account_set_show_all(field, TRUE);

	/* add the field to the group created above */
	purple_request_field_group_add_field(group, field);

	/* and finally we can create the request */
	purple_request_fields(purple_get_blist(), _("Listhandler - Copying"),
						_("Choose the account to copy from:"), NULL, request,
						_("C_opy"), G_CALLBACK(lh_migrate_source_request_cb),
						_("_Cancel"), NULL, NULL, NULL, NULL, NULL);
	return;
}

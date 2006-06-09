/*
 * Gaim Plugin Pack
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

static GList *buddies = NULL, *groups = NULL;
static GaimAccount *source_account = NULL, *target_account = NULL;
static const gchar *valid_target_prpl_id = NULL;

static gboolean
lh_migrate_filter(GaimAccount *account)
{
	/* get the protocol plugin id for the account passsed in */
	const gchar *prpl_id = gaim_account_get_protocol_id(account);

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
	GaimBuddyList *blist = gaim_get_blist();
	GaimBlistNode *root = blist->root, *g = NULL, *c = NULL, *b = NULL;

	/* walk the blist tree and build a list of the buddies and a list of
	 * the groups corresponding to each buddy */
	/* group level */
	for(g = root; g && GAIM_BLIST_NODE_IS_GROUP(g); g = g->next)
		/* contact level */
		for(c = g->child; c && GAIM_BLIST_NODE_IS_CONTACT(c); c = c->next)
			/* buddy level */
			for(b = c->child; b && GAIM_BLIST_NODE_IS_BUDDY(b); b = b-> next) {
				GaimGroup *tmp_group = gaim_group_new(((GaimGroup *)g)->name);
				GaimBuddy *tmp_buddy = (GaimBuddy *)b;

				/* if this buddy is on the source account then add it
				 * to the GLists */
				if(gaim_buddy_get_account(tmp_buddy) == source_account) {
					GaimBuddy *tmp_buddy_2 = gaim_buddy_new(target_account,
								  				gaim_buddy_get_name(tmp_buddy),
												gaim_buddy_get_alias(tmp_buddy));

					groups = g_list_prepend(groups, tmp_group);
					buddies = g_list_prepend(buddies, tmp_buddy_2);
				}
			}

	return;
}

static void
lh_migrate_target_request_cb(void *ignored, GaimRequestFields *fields)
{
	/* grab the target account from the request field */
	target_account = gaim_request_fields_get_account(fields,
													"migrate_target_acct");

	/* now build GLists of the buddies and corresponding groups */
	lh_migrate_build_lists();

	/* add the buddies to the Gaim buddy list */
	lh_util_add_to_blist(buddies, groups);

	/* add the buddies to the server-side list */
	gpp_add_buddies(target_account, buddies);

	/* now free the lists that were created */
	g_list_free(buddies);
	g_list_free(groups);

	return;
}

static void
lh_migrate_source_request_cb(void *ignored, GaimRequestFields *fields)
{
	GaimRequestFields *request;
	GaimRequestFieldGroup *group;
	GaimRequestField *field;

	source_account = gaim_request_fields_get_account(fields,
													"migrate_source_acct");
	valid_target_prpl_id = gaim_account_get_protocol_id(source_account);

	/* It seems Gaim is super-picky about the order of these first three calls */
	/* create a request */
	request = gaim_request_fields_new();

	/* now create a field group */
	group = gaim_request_field_group_new(NULL);
	/* and add that group to the request created above */
	gaim_request_fields_add_group(request, group);

	/* create a field */
	field = gaim_request_field_account_new("migrate_target_acct",
										_("Account"), NULL);

	/* mark the field as required */
	gaim_request_field_set_required(field, TRUE);

	/* filter this list so that only accounts with the same prpl as the
	 * source account can be chosen as a destination */
	gaim_request_field_account_set_filter(field, lh_migrate_filter);

	/* add the field to the group created above */
	gaim_request_field_group_add_field(group, field);

	/* and finally we can create the request */
	gaim_request_fields(gaim_get_blist(), _("Listhandler - Copying"),
						_("Choose the account to add buddies to:"), NULL,
						request, _("_Add"),
						G_CALLBACK(lh_migrate_target_request_cb), _("_Cancel"),
						NULL, NULL);

	return;
}

void
lh_migrate_action_cb(GaimPluginAction *action)
{
	GaimRequestFields *request;
	GaimRequestFieldGroup *group;
	GaimRequestField *field;

	/* It seems Gaim is super-picky about the order of these first three calls */
	/* create a request */
	request = gaim_request_fields_new();

	/* now create a field group */
	group = gaim_request_field_group_new(NULL);
	/* and add that group to the request created above */
	gaim_request_fields_add_group(request, group);

	/* create a field */
	field = gaim_request_field_account_new("migrate_source_acct",
										_("Account"), NULL);

	/* mark the field as required */
	gaim_request_field_set_required(field, TRUE);

	/* let's show offline accounts too */
	gaim_request_field_account_set_show_all(field, TRUE);

	/* add the field to the group created above */
	gaim_request_field_group_add_field(group, field);

	/* and finally we can create the request */
	gaim_request_fields(gaim_get_blist(), _("Listhandler - Copying"),
						_("Choose the account to copy from:"), NULL, request,
						_("C_opy"), G_CALLBACK(lh_migrate_source_request_cb),
						_("_Cancel"), NULL, NULL);
	return;
}

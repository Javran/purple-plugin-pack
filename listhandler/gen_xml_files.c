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

static const gchar *target_prpl_id = NULL;
static gchar *file_contents = NULL, *filename = NULL;
static gsize length;
static PurpleAccount *target_account = NULL, *source_account = NULL;
static PurpleBuddyList *buddies = NULL;
static PurpleConnection *gc = NULL;
static xmlnode *root = NULL;

static gboolean
lh_import_filter(PurpleAccount *account)
{
	const gchar *prpl_id = purple_account_get_protocol_id(account);

	if(!prpl_id)
		return FALSE;

	if(!strcmp(prpl_id, target_prpl_id))
		return TRUE;

	return FALSE;
}

static void
lh_generic_import_privacy(xmlnode *privacy)
{
	/* XXX: This is here awaiting Bleeter's privacy rewrite that will allow
	 *      importing and exporting of privacy options, lists, etc. */
}

static void
lh_generic_import_blist(xmlnode *blist)
{
	const gchar *group_name = NULL;
	PurpleGroup *purple_group = NULL;
	xmlnode *buddy = NULL;
	/* get the first group */
	xmlnode *group = xmlnode_get_child(blist, "group");

	while(group) {
		/* get the group's name */
		group_name = xmlnode_get_attrib(group, "name");

		purple_debug_info("listhandler: import", "Current group in XML is %s\n",
				group_name);

		/* create and/or get a pointer to the PurpleGroup */
		purple_group = purple_group_new(group_name);

		/* get the first buddy in this group */
		buddy = xmlnode_get_child(group, "buddy");

		while(buddy) {
			/* add the buddy to Purple's blist */
			lh_util_add_buddy(group_name, purple_group,
					xmlnode_get_attrib(buddy, "screenname"),
					xmlnode_get_attrib(buddy, "alias"), target_account);

			/* get the next buddy in the current group */
			buddy = xmlnode_get_next_twin(buddy);
		}

		/* get the next group in the exported blist */
		group = xmlnode_get_next_twin(group);
	}

	return;
}

static void
lh_generic_import_target_request_cb(void *ignored, PurpleRequestFields *fields)
{
	/* get the target account */
	target_account = purple_request_fields_get_account(fields, "generic_target_acct");

	purple_debug_info("listhandler: import",
			"Got the target account and its connection.\n");

	purple_debug_info("listhandler: import", "Beginning to parse XML.\n");

	/* call separate functions to import the privacy and blist */
	lh_generic_import_privacy(xmlnode_get_child(root, "privacy"));	
	lh_generic_import_blist(xmlnode_get_child(root, "blist"));

	purple_debug_info("listhandler: import", "Finished parsing XML.  "
			"Freeing allocated memory.\n");

	xmlnode_free(root);
}

static void
lh_generic_import_target_request(void)
{
	PurpleRequestFields *request;
	PurpleRequestFieldGroup *group;
	PurpleRequestField *field;
	GError *error = NULL;

	/* we need to make sure which gaim prpl this buddy list came from so we can
	 * filter the accounts list before showing it */

	/* read the file */
	g_file_get_contents(filename, &file_contents, &length, &error);

	root = xmlnode_from_str(file_contents, length);

	target_prpl_id = xmlnode_get_attrib(xmlnode_get_child(xmlnode_get_child(root, "config"),
				"prpl"), "id");

	purple_debug_info("listhandler: import", "Beginning Request API calls\n");

	/* It seems Purple is super-picky about the order of these first three calls */
	/* create a request */
	request = purple_request_fields_new();

	/* now create a field group */
	group = purple_request_field_group_new(NULL);
	/* and add that group to the request created above */
	purple_request_fields_add_group(request, group);

	/* create a field */
	field = purple_request_field_account_new("generic_target_acct", _("Account"), NULL);
	/* set the account field filter so we only see accounts with the same
	 * prpl as the blist was exported from */
	purple_request_field_account_set_filter(field, lh_import_filter);
	/* mark the field as required */
	purple_request_field_set_required(field, TRUE);

	/* add the field to the group created above */
	purple_request_field_group_add_field(group, field);

	/* and finally we can create the request */
	purple_request_fields(purple_get_blist(), _("Listhandler - Importing"),
						_("Choose the account to import to:"), NULL, request,
						_("_Import"),
						G_CALLBACK(lh_generic_import_target_request_cb),
						_("_Cancel"), NULL, NULL);

	purple_debug_info("listhandler: import", "Ending Request API calls\n");

	g_free(filename);

	return;
}

static void
lh_generic_import_request_cb(void *user_data, const char *file)
{
	purple_debug_info("listhandler: import", "Beginning import\n");

	if(file) {
		filename = g_strdup(file);

		lh_generic_import_target_request();
	}
}

static void
lh_generic_build_config_tree(xmlnode *parent)
{ /* we may need/want to expand the config area later for future feature
	 enhancements; this is why this tree gets its own building function. */

	xmlnode_set_attrib(xmlnode_new_child(parent, "config-version"),
			"version", "1");
	xmlnode_set_attrib(xmlnode_new_child(parent, "prpl"), "id",
			purple_account_get_protocol_id(source_account));
	xmlnode_set_attrib(xmlnode_new_child(parent, "source"), "account",
			purple_account_get_username(source_account));

	return;
}

static void
lh_generic_build_privacy_tree(xmlnode *parent)
{
	/* XXX: This function does nothing pending Bleeter's privacy rewrite, which
	 *      will allow exporting of privacy options, lists, etc. */
	return;
}

static void
lh_generic_build_blist_tree(xmlnode *parent)
{
	/*            root of tree           group      contact    buddy */
	PurpleBlistNode *root = buddies->root, *g = NULL, *c = NULL, *b = NULL;
	xmlnode *group = NULL, *buddy = NULL;
	PurpleBuddy *tmpbuddy = NULL;
	const char *tmpalias = NULL, *tmpname = NULL;

	/* iterate through the groups */
	for(g = root; g; g = g->next) {
		if(PURPLE_BLIST_NODE_IS_GROUP(g)) {
			const char *group_name = ((PurpleGroup *)g)->name;
			
			purple_debug_info("listhandler: export", "Node is group.  Name is: %s\n",
					group_name);

			/* add the group to the tree */
			group = xmlnode_new_child(parent, "group");
			xmlnode_set_attrib(group, "name", group_name);

			/* iterate through the contacts */
			for(c = g->child; c; c= c->next) {
				if(PURPLE_BLIST_NODE_IS_CONTACT(c)) {
					purple_debug_info("listhandler: export",
							"Node is contact.  Will parse its children.\n");

					/* iterate through the buddies */
					for(b = c->child; b && PURPLE_BLIST_NODE_IS_BUDDY(b); b = b->next) {
						tmpbuddy = (PurpleBuddy *)b;
						if(purple_buddy_get_account(tmpbuddy) == source_account) {
							tmpalias = purple_buddy_get_contact_alias(tmpbuddy);
							tmpname = purple_buddy_get_name(tmpbuddy);

							buddy = xmlnode_new_child(group, "buddy");
							xmlnode_set_attrib(buddy, "screenname", tmpname);

							if(strcmp(tmpalias, tmpname))
								xmlnode_set_attrib(buddy, "alias", tmpalias);
							else
								xmlnode_set_attrib(buddy, "alias", NULL);
						}
					}
				}
			}
		}
	}

	return;
}

static xmlnode *
lh_generic_build_tree(void)
{
	xmlnode *root_node = xmlnode_new("exported_buddy_list");

	/* since building this tree is really building three smaller trees that
	 * share a common parent, we'll build each tree separately to make this
	 * easier to read and understand what goes in each tree (hopefully). */
	lh_generic_build_config_tree(xmlnode_new_child(root_node, "config"));
	lh_generic_build_privacy_tree(xmlnode_new_child(root_node, "privacy"));
	lh_generic_build_blist_tree(xmlnode_new_child(root_node, "blist"));

	return root_node;
}

static void
lh_generic_export_request_cb(void *user_data, const char *filename)
{
	FILE *export = fopen(filename, "w");

	if(export) {
		int xmlstrlen = 0;
		xmlnode *tree = lh_generic_build_tree();
		char *xmlstring = xmlnode_to_formatted_str(tree, &xmlstrlen);

		purple_debug_info("listhandler: export",
				"XML tree built and converted to string.  String is:\n\n%s\n",
				xmlstring);

		fprintf(export, "%s\n", xmlstring);
		
		fclose(export);
		
		g_free(xmlstring);
		xmlnode_free(tree);
	} else
		purple_debug_info("listhandler: export", "Can't save file %s\n",
				filename ? filename : "NULL");
	
	return;
}

static void
lh_generic_export_cb(void *ignored, PurpleRequestFields *fields)
{
	/* get the source account from the dialog we requested */
	source_account = purple_request_fields_get_account(fields, "generic_source_acct");

	/* get the connection from the account */
	gc = purple_account_get_connection(source_account);

	/* this grabs the gaim buddy list, which will be walked thru later */
	buddies = purple_get_blist();

	if(buddies)
		purple_request_file(listhandler, _("Save Generic .blist File"), NULL,
				TRUE, G_CALLBACK(lh_generic_export_request_cb), NULL, NULL);
	else
		purple_debug_info("listhandler: export", "blist not returned\n");

	return;
}

void /* do some work and export the damn blist already */
lh_generic_export_action_cb(PurplePluginAction *action)
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
	field = purple_request_field_account_new("generic_source_acct", _("Account"), NULL);

	/* mark the field as required */
	purple_request_field_set_required(field, TRUE);

	/* let's show offline accounts too */
	purple_request_field_account_set_show_all(field, TRUE);

	/* add the field to the group created above */
	purple_request_field_group_add_field(group, field);

	/* and finally we can create the request */
	purple_request_fields(purple_get_blist(), _("Listhandler - Exporting"),
						_("Choose the account to export from:"), NULL, request,
						_("_Export"), G_CALLBACK(lh_generic_export_cb), _("_Cancel"),
						NULL, NULL);

	return;
}

void
lh_generic_import_action_cb(PurplePluginAction *action)
{
	purple_debug_info("listhandler: import", "Requesting the file.\n");

	purple_request_file(listhandler, _("Choose A Generic Buddy List File To Import"),
			NULL, FALSE, G_CALLBACK(lh_generic_import_request_cb), NULL, NULL);
	
	return;
}


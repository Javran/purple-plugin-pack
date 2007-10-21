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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301, USA.
 */

#include "listhandler.h"
#include "alias_xml_files.h"

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
lh_alist_import(xmlnode *alist)
{
	xmlnode *buddy = NULL;
	PurpleBuddy *temp = NULL;

	if(alist) {
		/* get the first buddy in the alias list */
		buddy = xmlnode_get_child(alist, "buddy");

		while(buddy) {
			temp = purple_find_buddy(target_account, xmlnode_get_attrib(buddy, "screenname") );

			if(temp != NULL) {
				purple_blist_alias_buddy(temp, xmlnode_get_attrib(buddy, "alias") );
				purple_debug_info("listhandler: import", "Added alias for %s\n",
					xmlnode_get_attrib(buddy, "screenname") );
			}	
			
			buddy = xmlnode_get_next_twin(buddy);
		}
	}
}

static void
lh_alist_build_alias_tree(xmlnode *parent)
{
	/* root of tree group contact buddy */
	PurpleBlistNode *root = buddies->root, *g = NULL, *c = NULL, *b = NULL;
	xmlnode *buddy = NULL;
	PurpleBuddy *tmpbuddy = NULL;
	const char *tmpalias = NULL, *tmpname = NULL;

	/* iterate through the groups */
	for(g = root; g; g = g->next) {
		if(PURPLE_BLIST_NODE_IS_GROUP(g)) {
			const char *group_name = ((PurpleGroup *)g)->name;
			
			purple_debug_info("listhandler: export", "Node is group.  Name is: %s\n",
					group_name);

			/* iterate through the contacts */
			for(c = g->child; c; c= c->next) {
				if(PURPLE_BLIST_NODE_IS_CONTACT(c)) {
					purple_debug_info("listhandler: export",
							"Node is contact.  Will parse its children.\n");

					/* iterate through the buddies */
					for(b = c->child; b && PURPLE_BLIST_NODE_IS_BUDDY(b); b = b->next) {
						tmpbuddy = (PurpleBuddy *)b;
						if(purple_buddy_get_account(tmpbuddy) == source_account) {
							tmpalias = purple_buddy_get_alias_only(tmpbuddy);
							if (tmpalias != NULL) {
								tmpname = purple_buddy_get_name(tmpbuddy);
								buddy = xmlnode_new_child(parent, "buddy");
								xmlnode_set_attrib(buddy, "screenname", tmpname);
								xmlnode_set_attrib(buddy, "alias", tmpalias);
							}
						}
					}
				}
			}
		}
	}

	return;
}

static void
lh_alist_build_config_tree(xmlnode *parent)
{ /* we may need to expand this area later for future feature ehnancements */

	xmlnode_set_attrib(xmlnode_new_child(parent, "config-version"), "version", "2");
	xmlnode_set_attrib(xmlnode_new_child(parent, "config-type"), "type", "alias-list");
	xmlnode_set_attrib(xmlnode_new_child(parent, "prpl"), "id",
			purple_account_get_protocol_id(source_account));
	xmlnode_set_attrib(xmlnode_new_child(parent, "source"), "account",
			purple_account_get_username(source_account));

	return;
}

static xmlnode *
lh_alist_build_tree(void)
{
	xmlnode *root_node = xmlnode_new("exported_alias_list");

	lh_alist_build_config_tree(xmlnode_new_child(root_node, "config"));
	lh_alist_build_alias_tree(xmlnode_new_child(root_node, "alist"));

	return root_node;
}

static void
lh_alist_export_request_cb(void *user_data, const char *filename)
{
	FILE *export = fopen(filename, "w");

	if(export) {
		int xmlstrlen = 0;
		xmlnode *tree = lh_alist_build_tree();
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
lh_alist_export_cb(void *ignored, PurpleRequestFields *fields)
{
	/* get the source account from the dialog we requested */
	source_account = purple_request_fields_get_account(fields, "generic_source_acct");

	/* get the connection from the account */
	gc = purple_account_get_connection(source_account);

	/* this grabs the purple buddy list, which will be walked thru later */
	buddies = purple_get_blist();

	if(buddies)
		purple_request_file(listhandler, _("Save Generic .alist File"), NULL,
				TRUE, G_CALLBACK(lh_alist_export_request_cb), NULL,
				source_account, NULL, NULL, NULL);
	else
		purple_debug_info("listhandler: export alias", "blist not returned\n");

	return;
}

void
lh_alist_export_action_cb(PurplePluginAction *action)
{
	PurpleRequestFields *request;
	PurpleRequestFieldGroup *group;
	PurpleRequestField *field;

	request = purple_request_fields_new();

	group = purple_request_field_group_new(NULL);
	purple_request_fields_add_group(request, group);

	field = purple_request_field_account_new("generic_source_acct", _("Account"), NULL);
	purple_request_field_set_required(field, TRUE);
	purple_request_field_account_set_show_all(field, TRUE);
	purple_request_field_group_add_field(group, field);

	purple_request_fields(purple_get_blist(), _("Listhandler - Exporting"),
						_("Choose the account to export from:"), NULL, request,
						_("_Export"), G_CALLBACK(lh_alist_export_cb), _("_Cancel"),
						NULL, NULL, NULL, NULL, NULL);

	return;
}

static void
lh_alist_import_target_request_cb(void *ignored, PurpleRequestFields *fields)
{
	/* get the target account */
	target_account = purple_request_fields_get_account(fields, "generic_target_acct");

	purple_debug_info("listhandler: import",
			"Got the target account and its connection.\n");

	purple_debug_info("listhandler: import", "Parsing Alias List in XML and setting aliases \n");

	lh_alist_import(xmlnode_get_child(root, "alist"));

	purple_debug_info("listhandler: import", "Finished setting aliases.  "
			"Freeing allocated memory.\n");

	xmlnode_free(root);
}

static void
lh_alist_import_target_request(void)
{
	PurpleRequestFields *request;
	PurpleRequestFieldGroup *group;
	PurpleRequestField *field;
	GError *error = NULL;

	/* we need to make sure which prpl this list came from so we
	 * can filter the accounts list before showing it */

	/* read the file and get the root xml node within it*/
	g_file_get_contents(filename, &file_contents, &length, &error);
	root = xmlnode_from_str(file_contents, length);

	target_prpl_id = xmlnode_get_attrib(xmlnode_get_child(xmlnode_get_child(root, "config"),
				"prpl"), "id");

	purple_debug_info("listhandler: import", "Beginning Request API calls\n");

	request = purple_request_fields_new();

	group = purple_request_field_group_new(NULL);
	purple_request_fields_add_group(request, group);

	field = purple_request_field_account_new("generic_target_acct", _("Account"), NULL);
	purple_request_field_account_set_filter(field, lh_import_filter);
	purple_request_field_set_required(field, TRUE);
	purple_request_field_group_add_field(group, field);

	purple_request_fields(purple_get_blist(), _("Listhandler - Importing"),
						_("Choose the account to import to:"), NULL, request,
						_("_Import"),
						G_CALLBACK(lh_alist_import_target_request_cb),
						_("_Cancel"), NULL, NULL, NULL, NULL, NULL);

	purple_debug_info("listhandler: import", "Ending Request API calls\n");

	g_free(filename);

	return;
}

static void
lh_alist_import_request_cb(void *user_data, const char *file)
{
	purple_debug_info("listhandler: import", "Beginning import\n");

	if(file) {
		filename = g_strdup(file);

		lh_alist_import_target_request();
	}
}


void
lh_alist_import_action_cb(PurplePluginAction *action)
{
	purple_debug_info("listhandler: import", "Requesting the file.\n");

	purple_request_file(listhandler, _("Choose A Generic Buddy List File To Import"),
			NULL, FALSE, G_CALLBACK(lh_alist_import_request_cb),
			NULL, NULL, NULL, NULL, NULL);
	
	return;
}

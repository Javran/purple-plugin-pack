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
#include "purple_blist_xml.h"

/*****************************************************************************
 * Structs
 ****************************************************************************/

typedef struct {
	/* important stuff for adding the buddies back */
	gchar *screenname;
	gchar *alias;
	const gchar *group;
	const gchar *account;
	const gchar *prpl_id;

	/* useful blistnode settings */
	gint signed_on;
	gint signed_off;
	gint lastseen;
	gint last_seen;
	gchar *gf_theme;
	gchar *icon_file;
	gchar *lastsaid;
	gchar *notes;

} LhPbxInfo;

/*****************************************************************************
 * Globals
 ****************************************************************************/
static GList *infolist = NULL;

/*****************************************************************************
 * "API"
 ****************************************************************************/
static LhPbxInfo *
lh_pbx_info_new(void)
{
	return g_new0(LhPbxInfo, 1);
}

static void
lh_pbx_info_destroy(LhPbxInfo *l)
{
	/* clean up the allocated memory returned by the xmlnode api */
	g_free(l->screenname);
	g_free(l->alias);
	g_free(l->gf_theme);
	g_free(l->icon_file);
	g_free(l->lastsaid);
	g_free(l->notes);

	/* now clean up the memory allocated for this struct */
	g_free(l);

	return;
}

/*****************************************************************************
 * Helpers
 ****************************************************************************/

static void
lh_pbx_import_cleanup(void)
{
	GList *tmp = infolist;

	for(; tmp; tmp = g_list_next(tmp))
		lh_pbx_info_destroy((LhPbxInfo *)(tmp->data));

	g_list_free(infolist);

	return;
}

static void
lh_pbx_import_file_parse(const char *file)
{
	GError *error = NULL;
	LhPbxInfo *tmpinfo = NULL;
	gchar *contents = NULL;
	gsize length = 0;
	xmlnode *root = NULL, *blist = NULL, *giter = NULL, *citer = NULL, *biter = NULL, *siter = NULL;

	/* grab the file contents, but bail out if there's an error */
	if(!g_file_get_contents(file, &contents, &length, &error)) {
		purple_debug_error("listhandler: import: blist.xml", "Error reading %s: %s\n",
				file ? file : "(null)", error->message ? error->message : "(null)");

		return;
	}

	/* if we get here, we have the file's contents--parse into xmlnode tree */
	root = xmlnode_from_str(contents, -1);

	blist = xmlnode_get_child(root, "blist");
	giter = xmlnode_get_child(blist, "group");

	for(; giter; giter = xmlnode_get_next_twin(giter)) {
		citer = xmlnode_get_child(giter, "contact");

		for(; citer; citer = xmlnode_get_next_twin(citer)) {
			biter = xmlnode_get_child(citer, "buddy");

			for(; biter; biter = xmlnode_get_next_twin(biter)) {
				tmpinfo = lh_pbx_info_new();
				siter = xmlnode_get_child(biter, "setting");

				tmpinfo->screenname = xmlnode_get_data(xmlnode_get_child(biter, "name"));
				tmpinfo->alias = xmlnode_get_data(xmlnode_get_child(biter, "alias"));
				tmpinfo->group = xmlnode_get_attrib(giter, "name");
				tmpinfo->account = xmlnode_get_attrib(biter, "account");
				tmpinfo->prpl_id = xmlnode_get_attrib(biter, "proto");

				for(; siter; siter = xmlnode_get_next_twin(siter)) {
					const gchar *setting_name = NULL;
					gchar *data = NULL;
					
					setting_name = xmlnode_get_attrib(siter, "name");
					data = xmlnode_get_data(siter);

					if(g_ascii_strcasecmp("signedon", setting_name))
						tmpinfo->signed_on = data ? atoi(data) : 0;
					else if(g_ascii_strcasecmp("signedoff", setting_name))
						tmpinfo->signed_off = data ? atoi(data) : 0;
					else if(g_ascii_strcasecmp("lastseen", setting_name))
						tmpinfo->lastseen = data ? atoi(data) : 0;
					else if(g_ascii_strcasecmp("last_seen", setting_name))
						tmpinfo->last_seen = data ? atoi(data) : 0;
					else if(g_ascii_strcasecmp("guifications-theme", setting_name))
						tmpinfo->gf_theme = data;
					else if(g_ascii_strcasecmp("buddy_icon", setting_name))
						tmpinfo->icon_file = data;
					else if(g_ascii_strcasecmp("lastsaid", setting_name))
						tmpinfo->lastsaid = data;
					else if(g_ascii_strcasecmp("notes", setting_name))
						tmpinfo->notes = data;
				}

				infolist = g_list_prepend(infolist, tmpinfo);
			}
		}
	}

	return;
}

/*****************************************************************************
 * Plugin Stuff
 ****************************************************************************/

static void
lh_pbx_import_add_buddies(void *ignored, PurpleRequestFields *fields)
{
	GList *tmp = infolist;
	LhPbxInfo *itmp = NULL;
	PurpleAccount *target = NULL;
	const gchar *target_name = NULL, *target_prpl = NULL;

	target = purple_request_fields_get_account(fields, "pbx_target_acct");

	target_name = purple_account_get_username(target);
	target_prpl = purple_account_get_protocol_id(target);

	purple_debug_info("listhandler: import", "Got target account: %s on %s\n",
			target_name, target_prpl);

	for(; tmp; tmp = tmp->next) {
		itmp = tmp->data;

		if(!strcmp(itmp->account, target_name) && !strcmp(itmp->prpl_id, target_prpl)) {
			purple_debug_info("listhandler: import",
					"Current entry in infolist matches target account!\n");

			lh_util_add_buddy(itmp->group, purple_group_new(itmp->group), itmp->screenname,
					itmp->alias, target, itmp->notes, itmp->signed_on, itmp->signed_off,
					itmp->lastseen, itmp->last_seen, itmp->gf_theme, itmp->icon_file,
					itmp->lastsaid);
		}

	}

	return;
}

static void
lh_pbx_import_target_request(void)
{
	PurpleRequestField *field = NULL;
	PurpleRequestFields *request = NULL;
	PurpleRequestFieldGroup *group = NULL;

	request = purple_request_fields_new();
	group = purple_request_field_group_new(NULL);

	purple_request_fields_add_group(request, group);

	field = purple_request_field_account_new("pbx_target_acct", _("Account"), NULL);

	purple_request_field_set_required(field, TRUE);
	purple_request_field_group_add_field(group, field);

	purple_request_fields(purple_get_blist(), _("Listhandler - Importing"),
			_("Choose the account whose buddy list you wish to restore:"),
			NULL, request, _("_Import"), G_CALLBACK(lh_pbx_import_add_buddies),
			_("_Cancel"), NULL, NULL, NULL, NULL, NULL);

	return;
}

static void
lh_pbx_import_request_cb(void *user_data, const char *file)
{
	purple_debug_info("listhandler: import", "In request callback\n");

	lh_pbx_import_file_parse(file);

	lh_pbx_import_target_request();

	lh_pbx_import_cleanup();

	return;
}

void
lh_pbx_import_action_cb(PurplePluginAction *action)
{
	purple_debug_info("listhandler: import", "Requesting the blist.xml to import\n");

	purple_request_file(listhandler, _("Choose a Libpurple blist.xml File To Import"),
			NULL, FALSE, G_CALLBACK(lh_pbx_import_request_cb), NULL, NULL, NULL, NULL, NULL);
	return;
}

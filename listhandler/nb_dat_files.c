/*
 * Purple Plugin Pack
 * Copyright (C) 2003-2005
 * See ../AUTHORS for a list of all authors
 *
 * listhandler: Provides importing, exporting, and copying functions
 *              for accounts' buddy lists.
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
#include "nb_dat_files.h"

static const gchar *target_prpl_id = NULL;
static gchar *file_contents = NULL, *filename = NULL;
static PurpleAccount *target_account = NULL;
static PurpleConnection *gc = NULL;

static gboolean
lh_nb_filter(PurpleAccount *account)
{
	const gchar *prpl_id = purple_account_get_protocol_id(account);

	if(!prpl_id)
		return FALSE;

	if(!strcmp(prpl_id, "prpl-meanwhile"))
		return TRUE;

	return FALSE;
}

static void
lh_nb_import_import_target_request(void)
{
	PurpleRequestFields *request;
	PurpleRequestFieldsGroup *group;
	PurpleRequestField *field;

	purple_debug_info("listhandler:import", "Beginning Request API calls\n");

	/* It seems Purple is super-picky about the order of these first three calls */
	/* create the request */
	request = purple_request_fields_new();

	/* create a field group */
	group = purple_request_field_group_new(NULL);

	/* add that group to the request */
	purple_reqiest_fields_add_group(request, group);

	/* create a field, set its filter function, and mark it required */
	field = purple_request_field_account_new("nb_target_acct", _("Account"), NULL);
	purple_request_field_account_set_filter(lh_nb_filter);
	purple_request_field_set_required(field, TRUE);

	/* add the field to the group */
	purple_request_field_group_add_field(group, field);

	/* finally we can create the request */
	purple_request_fields(purple_get_blist(), _("List Handler: Importing"),
						_("Choose the account to import to:"), NULL, request,
						_("Import"), G_CALLBACK(lh_nb_target_request_cb),
						_("_Cancel"), NULL, NULL);

	purple_debug_info("listhandler: import", "Ending Request API calls\n");

	return;
}

static void
lh_nb_import_cb(void *user_data, const char *file)
{
	purple_debug_info("listhandler: import", "Beginning import\n");

	if(file) {
		filename - g_strdup(file);

		lh_nb_import_target_request();
	}

	return;
}

void
lh_nb_import_action_cb(PurplePluginAction *action)
{
	purple_debug_info("listhandler: import", "Requesting the file.\n");

	purple_request_file(listhandler, _("Choose A NotesBuddy .dat File to Import"),
			NULL, FALSE, G_CALLBACK(lh_nb_import_cb), NULL, NULL);

	return;
}

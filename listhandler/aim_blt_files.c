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
#include "aim_blt_files.h"

static gchar *filename = NULL, *file_contents = NULL;
static gsize length;
static GString *bltfile_string = NULL;
static PurpleAccount *source_account = NULL, *target_account = NULL;
static PurpleBuddyList *buddies = NULL;
static PurpleConnection *gc = NULL;

static gboolean /* used to filter the account list to oscar accounts only */
lh_aim_filter(PurpleAccount *account)
{
	const gchar *prpl_id = purple_account_get_protocol_id(account);

	if(!prpl_id)
		return FALSE;

	if(!strcmp(prpl_id, "prpl-aim"))
		return TRUE;

	return FALSE;
}

static gchar * /* remove '{' and leading and trailing spaces from string */
lh_aim_str_normalize(gchar *s)
{
	/* replace all instances of the { and " characters with spaces, then
	 * strip whitespace */
	return g_strstrip(g_strdelimit(g_strdelimit(s, "\"", ' '), "{", ' '));
}

static gchar * /* extract alias from string by stripping AliasString and "s */
lh_aim_get_alias(gchar * s, gboolean v2)
{
	gint i, limit;

	/* Magic numbers: 18 = length of the string up to = for v2 files,
	 * 17 = length of the string up to "AliasString" for v1 files */
	if(v2) /* if a v2 file, we need to convert FriendlyName= to spaces */
		limit = 18;
	else /* else, v1 file, we need to convert AliasString to spaces */
		limit = 17;

	/* go through and kill off the chars that aren't part of the alias */
	for(i = 0; i < limit; i++)
		if(s[i] != ' ' && s[i] != '\0')
			s[i] = ' ';
	
	/* now strip the stupid, useless whitespace from the string */
	return g_strstrip(s);
}

static gchar ** /* read and split the file into manageable strings */
lh_aim_get_file_strings(gchar *file_contents, gsize *length, guint *strings_len)
{
	gchar **ret;
	GError *error = NULL;

	/* read the file as one bigass string */
	g_file_get_contents(filename, &file_contents, length, &error);

	if(error)
		purple_debug_misc("listhandler: import", "Error from glib:  %s\n",
				error->message);

	/* split that bigass string into manageable ones ;) */
	ret = g_strsplit(file_contents, "\n", 0);

	/* find out how many "manageable strings" we have */
	if(strings_len)
		*strings_len = g_strv_length(ret);

	if(error)
		g_error_free(error);

	g_free(filename);

	return ret; /* leave this stupid function already! */
}

static void /* find where the buddy list begins and ends */
lh_aim_list_find(gchar **strings, guint strings_len, guint *begin, guint *end)
{
	int i;

	/* this is a bit ugly but it works */
	for(i = 0; i < strings_len; i++) {
		if(!strncmp(strings[i], " list {", 7))
			*begin = i;
		if(*begin && i > *begin && !strncmp(strings[i], " }", 2)) {
			*end = i;
			break;
		}
	}

	return;
}

static void /* parse this damn buddy list already */
lh_aim_list_parse_and_add(gchar **strings, guint length, guint begin, guint end)
{
	gchar *current_group = NULL, *current_buddy = NULL, *current_alias = NULL;
	gint i, current_group_begin = 0, current_group_end = 0;
	PurpleGroup *current_purple_group = NULL;
	PurpleBuddy *tmpbuddy = NULL;
	GList *buddies = NULL, *groups = NULL;

	/* loop until we find the end of the buddy list */
	while(current_group_end < end && current_group_end != end - 1) {
		purple_debug_info("listhandler: import", "Started the parsing loop\n");

		/* it's safe to start one after and end one before the start and end
		 * of the list section, so save the two iterations.  the if statement
		 * determines if we've already been through at least one group or not
		 * and sets i accordingly to prevent missing or reparsing a group */
		if(current_group_end > 0)
			i = current_group_end + 1;
		else
			i = begin + 1;

		/* pass through the list from the starting point determined above
		 * until the end of the current group's section in the blt file is
		 * found */
		for(; i < end; i++) {
			if(!strncmp(strings[i], "  ", 2) && strlen(strings[i]) >= 3 &&
					strings[i][2] != ' ' && strings[i][2] != '}')
				current_group_begin = i;

			if(!strncmp(strings[i], "  }", 3)) {
				current_group_end = i;
				break;
			}
		}

		purple_debug_info("listhandler: import", "Current group begins %d, ends %d\n",
				current_group_begin, current_group_end);

		/* now strip {, ", and whitespace from the group and keep an extra
		 * pointer to it around for easy access */
		current_group = lh_aim_str_normalize(strings[current_group_begin]);

		/* create a PurpleGroup and add to the list.  This is surprisingly easy. */
		current_purple_group = purple_group_new(current_group);
		purple_blist_add_group(current_purple_group, NULL);
	
		/* now parse the actual group */
		for(i = current_group_begin + 1; i < current_group_end; i++) {
			if(!strncmp(strings[i], "   ", 3) && strlen(strings[i]) >= 4 &&
					strings[i][3] != ' ' && strings[i][3] != '}')
			{
				/* this is the buddy name; keep extra pointer for easy access */
				current_buddy = lh_aim_str_normalize(strings[i]);

				/* since the geniuses that designed the blt format decided
				 * that "M y S cr ee nn a m e" is acceptable in their blt files,
				 * I have to work around their incompetence */
				lh_aim_str_normalize(current_buddy);

				purple_debug_info("listhandler: import", "current buddy is %s\n",
						current_buddy);

				/* test to see if the buddy has an alias set */
				if(!strncmp(strings[i + 1], "    AliasKey {", 14) &&
					!strncmp(strings[i + 2], "     AliasString ", 17))
				{
					/* grab the alias */
					current_alias = lh_aim_get_alias(strings[i + 2], FALSE);
					i += 2; /* advance counter to prevent reparsing the alias */
				} else if(!strncmp(strings[i + 1], "    FriendlyName=", 17)) {
					/* Version 2 .blt format uses FriendlyName= to denote an alias */
					/* grab the alias */
					current_alias = lh_aim_get_alias(strings[i + 1], TRUE);
					i++; /* advance the counter to prevent reparsing the alias */
				} else /* no alias is set */
					current_alias = NULL;

				tmpbuddy = purple_buddy_new(target_account, current_buddy,
										current_alias);
				purple_debug_info("listhandler: import",
						"new PurpleBuddy created: %s, %s, %s\n", current_buddy,
						current_alias ? current_alias : "NULL",
						purple_account_get_username(target_account));

				if(tmpbuddy && current_purple_group) {
					buddies = g_list_prepend(buddies, tmpbuddy);
					groups = g_list_prepend(groups, current_purple_group);

					purple_debug_info("listhandler: import", "added current "
							"buddy to the GLists\n");
				}
			}
		}
	}

	if(buddies && groups) {
		lh_util_add_to_blist(buddies, groups);
#if PURPLE_VERSION_CHECK(3,0,0)
		purple_account_add_buddies(target_account, buddies, NULL);
#else
		purple_account_add_buddies(target_account, buddies);
#endif
	} else {
		if(!buddies && !groups)
			purple_debug_info("listhandler: import", "BOTH GLISTS NULL!!!!!\n");
		if(!buddies)
			purple_debug_info("listhandler: import", "BUDDY GLIST NULL!!!\n");
		if(!groups)
			purple_debug_info("listhandler: import", "GROUP GLIST NULL!!!!\n");
	}

	return;
}

static void
lh_aim_import_target_request_cb(void *ignored, PurpleRequestFields *fields)
{
	gchar **strings = NULL;
	guint strings_len = 0, list_begin = 0, list_end = 0;

	/* get the target account from the dialog we requested */
	target_account = purple_request_fields_get_account(fields,
													"aim_target_acct");

	/* read and split the file */
	strings = lh_aim_get_file_strings(file_contents, &length, &strings_len);

	/* find the list in that crapload of memory that just got allocated */
	lh_aim_list_find(strings, strings_len, &list_begin, &list_end);

	purple_debug_info("listhandler: import", "List begins at %d; ends at %d\n",
			list_begin, list_end);

	/* parse the freaking list already */
	lh_aim_list_parse_and_add(strings, strings_len, list_begin, list_end);

	/* clean up all that crap that got allocated */
	g_strfreev(strings);
	g_free(file_contents);

	return;
}

static void /* does the request API calls needed */
lh_aim_import_target_request(void)
{
	PurpleRequestFields *request;
	PurpleRequestFieldGroup *group;
	PurpleRequestField *field;

	purple_debug_info("listhandler: import", "Beginning Request API calls\n");

	/* It seems Purple is super-picky about the order of these first three calls */
	/* create a request */
	request = purple_request_fields_new();

	/* now create a field group */
	group = purple_request_field_group_new(NULL);
	/* and add that group to the request created above */
	purple_request_fields_add_group(request, group);

	/* create a field */
	field = purple_request_field_account_new("aim_target_acct",
										_("Account"), NULL);
	/* set the account field filter so we only see oscar accounts */
	purple_request_field_account_set_filter(field, lh_aim_filter);
	/* mark the field as required */
	purple_request_field_set_required(field, TRUE);

	/* add the field to the group created above */
	purple_request_field_group_add_field(group, field);

	/* and finally we can create the request */
	purple_request_fields(purple_get_blist(), _("List Handler: Importing"),
						_("Choose the account to import to:"), NULL,
						request, _("_Import"),
						G_CALLBACK(lh_aim_import_target_request_cb),
						_("_Cancel"), NULL, NULL, NULL, NULL, NULL);

	purple_debug_info("listhandler: import", "Ending Request API calls\n");

	return;
}

static void
lh_aim_import_cb(void *user_data, const char *file)
{
	purple_debug_info("listhandler: import", "Beginning import\n");

	if(file) {
		filename = g_strdup(file);

		lh_aim_import_target_request();
	}

	return;
}

static void
lh_aim_string_add_buddy(PurpleBlistNode *node)
{
	PurpleBuddy *buddy = (PurpleBuddy *)node;
	const char *tmpalias = purple_buddy_get_contact_alias(buddy),
			   *tmpname = purple_buddy_get_name(buddy);

	purple_debug_info("listhandler: export", "Node is buddy.  Name is: %s\n", tmpname);

	/* only export if the buddy is on the right account */
	if(purple_buddy_get_account(buddy) == source_account) {
		/* add the buddy's screenname to the string */
		g_string_append_printf(bltfile_string, "   \"%s\"", tmpname);

		/* if the alias is NOT the same as the screenname, add it to the string */
		if(strcmp(tmpalias, tmpname))
			g_string_append_printf(bltfile_string,
					" {\n    AliasKey {\n     \"%s\"\n    }\n   }\n",
					tmpalias );
		else /* otherwise we're done with this buddy */
			g_string_append_printf(bltfile_string, "\n");
	}

	return;
}

static void
lh_aim_build_string(void)
{
	PurpleBlistNode *root_node = buddies->root, *g = NULL,
				  *c = NULL, *b = NULL;

	bltfile_string = g_string_new("Config {\n version 1\n}\n");

	g_string_append_printf(bltfile_string, "User {\n screenname %s\n}\n",
			purple_account_get_username(source_account));
	g_string_append(bltfile_string, "Buddy {\n list {\n");

	/* this outer loop iterates through the group level of the tree */
	for(g = root_node; g && PURPLE_BLIST_NODE_IS_GROUP(g); g = g->next)
	{
		purple_debug_info("listhandler: export", "Node is group.  Name is: %s\n",
				((PurpleGroup *)g)->name);

		/* add the group to the string */
		g_string_append_printf(bltfile_string, "  \"%s\" {\n",
				((PurpleGroup *)g)->name);

		/* iterate through the contact level in this group */
		for(c = g->child; c && PURPLE_BLIST_NODE_IS_CONTACT(c); c = c->next) {
			purple_debug_info("listhandler: export",
					"Node is contact.  Will parse its children.\n");

			/* iterate through the contact's buddies */
			for(b = c->child; b && PURPLE_BLIST_NODE_IS_BUDDY(b); b = b->next)
				lh_aim_string_add_buddy(b);
		}

			g_string_append(bltfile_string, "  }\n");
	}
	
	/* finish the string we'll dump to the file */
	g_string_append(bltfile_string, " }\n}\n");

	purple_debug_info("listhandler: export", "String built.  String is:\n\n%s\n",
			bltfile_string->str);

	return;
}

static void
lh_aim_export_request_cb(void *user_data, const char *filename)
{
	FILE *export = fopen(filename, "w");

	if(export) {
		lh_aim_build_string();
		fprintf(export, "%s", bltfile_string->str);
		fclose(export);
	} else
		purple_debug_info("listhandler: export", "Can't save file %s\n",
				filename ? filename : "NULL");

	g_string_free(bltfile_string, TRUE);

	return;
}

static void
lh_aim_export_cb(void *ignored, PurpleRequestFields *fields)
{
	/* get the source account from the dialog we requested */
	source_account = purple_request_fields_get_account(fields,
													"aim_source_acct");

	/* get the connection from the account */
	gc = purple_account_get_connection(source_account);

	/* this grabs the purple buddy list, which will be walked thru later */
	buddies = purple_get_blist();

	if(buddies)
		purple_request_file(listhandler, _("Save AIM .blt File"), NULL, TRUE,
				G_CALLBACK(lh_aim_export_request_cb), NULL,
				source_account, NULL, NULL, NULL);
	else
		purple_debug_info("listhandler: export", "blist not returned\n");

	return;
}

void /* do some work and export the damn blist already */
lh_aim_export_action_cb(PurplePluginAction *action)
{
	PurpleRequestFields *request;
	PurpleRequestFieldGroup *group;
	PurpleRequestField *field;

	purple_debug_info("listhandler: export", "Beginning Request API calls\n");

	/* It seems Purple is super-picky about the order of these first three calls */
	/* create a request */
	request = purple_request_fields_new();

	/* now create a field group */
	group = purple_request_field_group_new(NULL);
	/* and add that group to the request created above */
	purple_request_fields_add_group(request, group);

	/* create a field */
	field = purple_request_field_account_new("aim_source_acct",
										_("Account"), NULL);
	/* set the account field filter so we only see oscar accounts */
	purple_request_field_account_set_filter(field, lh_aim_filter);
	/* mark the field as required */
	purple_request_field_set_required(field, TRUE);

	/* add the field to the group created above */
	purple_request_field_group_add_field(group, field);

	/* and finally we can create the request */
	purple_request_fields(purple_get_blist(), _("List Handler: Exporting"),
						_("Choose the account to export from:"), NULL, request,
						_("_Export"), G_CALLBACK(lh_aim_export_cb), _("_Cancel"),
						NULL, NULL, NULL, NULL, NULL);

	purple_debug_info("listhandler: export", "Ending Request API calls\n");

	return;
}

void
lh_aim_import_action_cb(PurplePluginAction *action)
{
	purple_debug_info("listhandler: import", "Requesting the file.\n");

	purple_request_file(listhandler, _("Choose An AIM .blt File To Import"),
			NULL, FALSE, G_CALLBACK(lh_aim_import_cb),
			NULL, NULL, NULL, NULL, NULL);

	return;
}

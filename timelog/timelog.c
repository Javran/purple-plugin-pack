/*
 * TimeLog plugin.
 *
 * Copyright (C) 2006 Jon Oberheide
 * Copyright (C) 2007-2008 Stu Tomlinson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

/* If you can't figure out what this line is for, DON'T TOUCH IT. */
#include "../common/pp_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>

#include <plugin.h>
#include <request.h>
#include <log.h>
#include <util.h>
#include <debug.h>
#include <version.h>

#include <gtkplugin.h>

#include "timelog.h"
#include "log-widget.h"
#include "range-widget.h"

typedef struct {
	PurpleAccount *account;
	GList *logs;
	time_t start;
	time_t end;
} log_query_t;

static void
foreach_log(gpointer value, gpointer data)
{
	PurpleLog *log = (PurpleLog *) value;
	log_query_t *query = (log_query_t *) data;

	if (log->time >= query->start && log->time <= query->end) {
		query->logs = g_list_append(query->logs, log);
	}
}

static void
foreach_log_set(gpointer key, gpointer value, gpointer data)
{
	GList *logs;
	PurpleLogSet *set = (PurpleLogSet *) value;
	log_query_t *query = (log_query_t *) data;

	if (query->account != set->account) {
		return;
	}

	logs = purple_log_get_logs(set->type, set->name, set->account);

	g_list_foreach(logs, foreach_log, query);
}

static void
cb_select_time(gpointer data, PurpleRequestFields *fields)
{
	GHashTable *log_sets;
	GtkWidget *range_dialog;
	log_query_t *query;

	query = g_new0(log_query_t, 1);
	query->account = purple_request_fields_get_account(fields, "acct");

	range_dialog = range_widget_create();

	if (gtk_dialog_run(GTK_DIALOG(range_dialog)) == GTK_RESPONSE_OK) {
		range_widget_get_bounds(range_dialog, &query->start, &query->end);

		log_sets = purple_log_get_log_sets();
		g_hash_table_foreach(log_sets, foreach_log_set, query);

		tl_debug("found %u logs for %s between %lu and %lu\n", 
				g_list_length(query->logs),
				query->account->username,
				query->start, query->end);

		log_widget_display_logs(query->logs);

		g_hash_table_destroy(log_sets);
	}

	range_widget_destroy(range_dialog);

	g_free(query);
}

static void
cb_select_account(PurplePluginAction *action)
{
	PurpleRequestFields *request;
	PurpleRequestFieldGroup *group;
	PurpleRequestField *field;

	group = purple_request_field_group_new(NULL);

	field = purple_request_field_account_new("acct", "Account", NULL);
	purple_request_field_account_set_show_all(field, TRUE);
	purple_request_field_group_add_field(group, field);

	request = purple_request_fields_new();
	purple_request_fields_add_group(request, group);

	purple_request_fields(action->plugin, TIMELOG_TITLE,
			_("Select account to view logs for:"), NULL, request,
			_("Select Account"), G_CALLBACK(cb_select_time),
			_("Cancel"), NULL, NULL, NULL, NULL, NULL);
}

static GList *
actions(PurplePlugin *plugin, gpointer context)
{
	GList *l = NULL;
	PurplePluginAction *act = NULL;

	act = purple_plugin_action_new(_("Select Account/Time"), cb_select_account);
	l = g_list_append(l, act);

	return l;
}

static gboolean
load_plugin(PurplePlugin *plugin)
{
	return TRUE;
}

static gboolean
unload_plugin(PurplePlugin *plugin)
{
	return TRUE;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,				/**< type	*/
	PIDGIN_PLUGIN_TYPE,				/**< ui_req	*/
	0,						/**< flags	*/
	NULL,						/**< deps	*/
	PURPLE_PRIORITY_DEFAULT,			/**< priority	*/
	TIMELOG_PLUGIN_ID,				/**< id		*/
	NULL,						/**< name	*/
	PP_VERSION,					/**< version	*/
							/**  summary	*/
	N_("Allows the viewing of Pidgin logs within a specific time range"),
							/**  desc	*/
	N_("Allows the viewing of Pidgin logs within a specific time range"),
	"Jon Oberheide <jon@oberheide.org>",		/**< author	*/
	"http://jon.oberheide.org/projects/gaim-timelog/",
							/**< homepage	*/
	load_plugin,					/**< load	*/
	unload_plugin,					/**< unload	*/
	NULL,						/**< destroy	*/
	NULL,						/**< ui_info	*/
	NULL,						/**< extra_info	*/
	NULL,						/**< pref info	*/
	actions
};

static void
init_plugin(PurplePlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif

	info.name = TIMELOG_TITLE;
	info.summary = _(info.summary);
	info.description = _(info.description);
}

PURPLE_INIT_PLUGIN(timelog, init_plugin, info)

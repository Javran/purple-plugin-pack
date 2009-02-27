/*
 * XMPP Priority - Add Account Options to adjust the default priority values
 *                 for XMPP accounts.
 * Copyright (C) 2009 Paul Aurich <paul@aurich.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02111-1301, USA.
 */

#include "../common/pp_internal.h"

#define PLUGIN_STATIC_NAME  "xmppprio"
#define PLUGIN_ID           "core-darkrain42-" PLUGIN_STATIC_NAME
#define PLUGIN_AUTHOR       "Paul Aurich <paul@aurich.com>"

/* Purple headers */
#include <account.h>
#include <accountopt.h>
#include <plugin.h>
#include <status.h>

/* Constants */
#define XMPP_PLUGIN_ID "prpl-jabber"

#define DEFAULT_AVAIL_PRIO 1
#define DEFAULT_AWAY_PRIO  0

static void
set_account_priorities(PurpleAccount *account, const int avail_prio,
                       const int away_prio)
{
	PurplePresence *presence;
	GList *statuses;

	/*
	 * This is disgusting. Basically, PurpleStatusType contains the PurpleValues
	 * representing the default values of each of the status attributes. Go in
	 * and update the defaults for 'priority' for each status type.
	 */
	presence = purple_account_get_presence(account);
	statuses = purple_presence_get_statuses(presence);
	for ( ; statuses; statuses = statuses->next) {
		PurpleStatus *status = statuses->data;
		PurpleStatusType *type = purple_status_get_type(status);
		PurpleStatusAttr *attr;
		PurpleValue *default_value;

		if (!purple_status_type_is_exclusive(type))
			continue;

		/*
		 * A priority for offline makes no sense. Technically, this status type
		 * has no priority attribute, but in case that changes, neither of these
		 * priority values should apply to it.
		 */
		if (PURPLE_STATUS_OFFLINE == purple_status_type_get_primitive(type))
			continue;

		attr = purple_status_type_get_attr(type, "priority");
		if (!attr)
			continue;

		default_value = purple_status_attr_get_value(attr);
		if (default_value->type == PURPLE_TYPE_INT) {
			if (purple_status_type_is_available(type))
				purple_value_set_int(default_value, avail_prio);
			else
				purple_value_set_int(default_value, away_prio);
		}
	}
}

static void
signing_on_cb(PurpleConnection *pc)
{
	PurpleAccount *account;
	int avail_prio, away_prio;

	g_return_if_fail(NULL != pc);

	account = purple_connection_get_account(pc);
	g_return_if_fail(NULL != account);

	if (!g_str_equal(purple_account_get_protocol_id(account), XMPP_PLUGIN_ID))
		return;

	avail_prio = purple_account_get_int(account, PLUGIN_ID "_avail_prio", DEFAULT_AVAIL_PRIO);
	away_prio  = purple_account_get_int(account, PLUGIN_ID "_away_prio", DEFAULT_AWAY_PRIO);
	set_account_priorities(account, avail_prio, away_prio);
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	PurplePlugin *xmpp_prpl;
	PurplePluginProtocolInfo *prpl_info;
	PurpleAccountOption *option;
	GList *list;

	xmpp_prpl = purple_plugins_find_with_id(XMPP_PLUGIN_ID);
	if (NULL == xmpp_prpl)
		return FALSE;

	prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(xmpp_prpl);
	if (NULL == prpl_info)
		return FALSE;

	/* Register protocol preferences */
	option = purple_account_option_int_new(_("Available Priority"),
	                                       PLUGIN_ID "_avail_prio", DEFAULT_AVAIL_PRIO);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);

	option = purple_account_option_int_new(_("Away Priority"),
	                                       PLUGIN_ID "_away_prio", DEFAULT_AWAY_PRIO);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);

	/* Connect signals */
	purple_signal_connect(purple_connections_get_handle(), "signing-on",
	                      plugin, PURPLE_CALLBACK(signing_on_cb), NULL);

	/* Set the priorities for all accounts */
	for (list = purple_accounts_get_all(); list; list = list->next) {
		PurpleAccount *account = list->data;
		if (g_str_equal(purple_account_get_protocol_id(account), XMPP_PLUGIN_ID)) {
			int avail_prio = purple_account_get_int(account, PLUGIN_ID "_avail_prio",
			                                        DEFAULT_AVAIL_PRIO);
			int away_prio  = purple_account_get_int(account, PLUGIN_ID "_away_prio",
			                                        DEFAULT_AWAY_PRIO);
			set_account_priorities(account, avail_prio, away_prio);
		}
	}

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	PurplePlugin *xmpp_prpl;
	PurplePluginProtocolInfo *prpl_info;
	GList *list;

	/* Disconnect signals */
	purple_signals_disconnect_by_handle(plugin);

	xmpp_prpl = purple_plugins_find_with_id(XMPP_PLUGIN_ID);
	if (NULL == xmpp_prpl)
		return FALSE;

	prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(xmpp_prpl);
	if (NULL == prpl_info)
		return FALSE;

	list = prpl_info->protocol_options;
	while (NULL != list) {
		PurpleAccountOption *opt = list->data;
		if (g_str_has_prefix(purple_account_option_get_setting(opt), PLUGIN_ID "_")) {
			list = g_list_delete_link(list, list);
			purple_account_option_destroy(opt);
		} else
			list = list->next;
	}

	/* Reset the priorities for accounts */
	for (list = purple_accounts_get_all(); list; list = list->next) {
		PurpleAccount *account = list->data;
		if (g_str_equal(purple_account_get_protocol_id(account), XMPP_PLUGIN_ID))
			set_account_priorities(account, DEFAULT_AVAIL_PRIO, DEFAULT_AWAY_PRIO);
	}
	return TRUE;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,			/* Magic				*/
	PURPLE_MAJOR_VERSION,			/* Purple Major Version	*/
	PURPLE_MINOR_VERSION,			/* Purple Minor Version	*/
	PURPLE_PLUGIN_STANDARD,			/* plugin type			*/
	NULL,							/* ui requirement		*/
	0,								/* flags				*/
	NULL,							/* dependencies			*/
	PURPLE_PRIORITY_DEFAULT,		/* priority				*/

	PLUGIN_ID,						/* plugin id			*/
	NULL,							/* name					*/
	PP_VERSION,						/* version				*/
	NULL,							/* summary				*/
	NULL,							/* description			*/
	PLUGIN_AUTHOR,					/* author				*/
	PP_WEBSITE,						/* website				*/

	plugin_load,					/* load					*/
	plugin_unload,					/* unload				*/
	NULL,							/* destroy				*/

	NULL,							/* ui_info				*/
	NULL,							/* extra_info			*/
	NULL,							/* prefs_info			*/
	NULL,							/* actions				*/
	NULL,							/* reserved 1			*/
	NULL,							/* reserved 2			*/
	NULL,							/* reserved 3			*/
	NULL							/* reserved 4			*/
};

static void
init_plugin(PurplePlugin *plugin)
{
	info.dependencies = g_list_prepend(info.dependencies, XMPP_PLUGIN_ID);

#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("XMPP Priority");
	info.summary = _("Adjust the priorities of XMPP statuses");
	info.description = _("Adds account options that allow users to specify the "
	                     "priorities used for available and away priorities "
	                     "for XMPP accounts.");
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info)

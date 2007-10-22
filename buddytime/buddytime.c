/*
 * Buddy Time - Displays a buddy's local time
 *
 * A libpurple plugin that allows you to configure a timezone on a per-contact
 * basis so it can display the localtime of your contact when a conversation
 * starts. Convenient if you deal with contacts from many parts of the
 * world.
 *
 * Copyright (C) 2006-2007, Richard Laager <rlaager@pidgin.im>
 * Copyright (C) 2006, Martijn van Oosterhout <kleptog@svana.org>
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
#include "../common/pp_internal.h"

#include "buddytime.h"
#define PLUGIN_STATIC_NAME   CORE_PLUGIN_STATIC_NAME
#define PLUGIN_ID            CORE_PLUGIN_ID

#define SETTING_NAME "timezone"
#define CONTROL_NAME PLUGIN_ID "-" SETTING_NAME

#include <glib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

#include "conversation.h"
#include "core.h"
#include "debug.h"
#include "notify.h"
#include "plugin.h"
#include "request.h"
#include "util.h"
#include "value.h"
#include "version.h"

#include "gtkblist.h"

#include "recurse.h"

#define TIMEZONE_FLAG ((void*)1)
#define DISABLED_FLAG ((void*)2)

BuddyTimeUiOps *ui_ops = NULL;

static PurplePlugin *plugin_self;

/* Resolve specifies what the return value should mean:
 *
 * If TRUE, it's for display, we want to know the *effect* thus hiding the
 * "none" value and going to going level to find the default
 *
 * If false, we only want what the user enter, thus the string "none" if
 * that's what it is
 *
 * data is here so we can use this as a callback for IPC
 */
static const char *
buddy_get_timezone(PurpleBlistNode * node, gboolean resolve, void *data)
{
    PurpleBlistNode *datanode = NULL;
    const char *timezone;

    switch (node->type)
    {
        case PURPLE_BLIST_BUDDY_NODE:
            datanode = (PurpleBlistNode *) purple_buddy_get_contact((PurpleBuddy *) node);
            break;
        case PURPLE_BLIST_CONTACT_NODE:
            datanode = node;
            break;
        case PURPLE_BLIST_GROUP_NODE:
            datanode = node;
            break;
        default:
            return NULL;
    }

    timezone = purple_blist_node_get_string(datanode, SETTING_NAME);

    if (!resolve)
        return timezone;

    /* The effect of "none" is to stop recursion */
    if (timezone && strcmp(timezone, "none") == 0)
        return NULL;

    if (timezone)
        return timezone;

    if (datanode->type == PURPLE_BLIST_CONTACT_NODE)
    {
        /* There is no purple_blist_contact_get_group(), though there probably should be */
        datanode = datanode->parent;
        timezone = purple_blist_node_get_string(datanode, SETTING_NAME);
    }

    if (timezone && strcmp(timezone, "none") == 0)
        return NULL;

    return timezone;
}

/* Calcuates the difference between two struct tm's. */
static double
timezone_calc_difference(struct tm *remote_tm, struct tm *tmp_tm)
{
    int hours_diff = 0;
    int minutes_diff = 0;

    /* Note this only works because the times are
     * known to be within 24 hours of each other! */
    if (remote_tm->tm_mday != tmp_tm->tm_mday)
        hours_diff = 24;

    hours_diff   += (remote_tm->tm_hour - tmp_tm->tm_hour);
    minutes_diff  = (remote_tm->tm_min  - tmp_tm->tm_min);

    return (double)minutes_diff / 60.0 + hours_diff;
}

/* data is here so we can use this as a callback for IPC */
static int
timezone_get_time(const char *timezone, struct tm *tm, double *diff, void *data)
{
    time_t now;
    struct tm *tm_tmp;
#ifdef PRIVATE_TZLIB
    struct state *tzinfo = timezone_load(timezone);

    if(!tzinfo)
        return -1;

    time(&now);
    localsub(&now, 0, tm, tzinfo);
    free(tzinfo);
#else
    const gchar *old_tz;

    /* Store the current TZ value. */
    old_tz = g_getenv("TZ");

    g_setenv("TZ", timezone, TRUE);

    time(&now);
    tm_tmp = localtime(&now);
    *tm = *tm_tmp;              /* Must copy, localtime uses local buffer */

    /* Reset the old TZ value. */
    if (old_tz == NULL)
        g_unsetenv("TZ");
    else
        g_setenv("TZ", old_tz, TRUE);
#endif

    /* Calculate user's localtime, and compare. If same, no output */
    tm_tmp = localtime(&now);

    if (tm_tmp->tm_hour == tm->tm_hour && tm_tmp->tm_min == tm->tm_min)
        return 1;

    *diff = timezone_calc_difference(tm, tm_tmp);
    return 0;
}

static void
timezone_createconv_cb(PurpleConversation * conv, void *data)
{
    const char *name;
    PurpleBuddy *buddy;
    struct tm tm;
    const char *timezone;
    double diff;
    int ret;

    if (purple_conversation_get_type(conv) != PURPLE_CONV_TYPE_IM)
        return;

    name = purple_conversation_get_name(conv);
    buddy = purple_find_buddy(purple_conversation_get_account(conv), name);
    if (!buddy)
        return;

    timezone = buddy_get_timezone((PurpleBlistNode *) buddy, TRUE, NULL);

    if (!timezone)
        return;

    ret = timezone_get_time(timezone, &tm, &diff, NULL);

    if (ret == 0)
    {
        const char *text = purple_time_format(&tm);

        char *str;
	if (diff < 0)
	{
            diff = 0 - diff;
            str = g_strdup_printf(dngettext(GETTEXT_PACKAGE,
                                            "Remote Local Time: %s (%.4g hour behind)",
                                            "Remote Local Time: %s (%.4g hours behind)", diff),
                                  text, diff);
	}
	else
	{
            str = g_strdup_printf(dngettext(GETTEXT_PACKAGE,
                                            "Remote Local Time: %s (%.4g hour ahead)",
                                            "Remote Local Time: %s (%.4g hours ahead)", diff),
                                  text, diff);
	}

        purple_conversation_write(conv, PLUGIN_STATIC_NAME, str, PURPLE_MESSAGE_SYSTEM, time(NULL));

        g_free(str);
    }
}

#if 0
static void
buddytimezone_submitfields_cb(PurpleRequestFields * fields, PurpleBlistNode * data)
{
    PurpleBlistNode *node;
    PurpleRequestField *list;

    /* timezone stuff */
    purple_debug(PURPLE_DEBUG_INFO, PLUGIN_STATIC_NAME, "buddytimezone_submitfields_cb(%p,%p)\n", fields, data);

    switch (data->type)
    {
        case PURPLE_BLIST_BUDDY_NODE:
            node = (PurpleBlistNode *) purple_buddy_get_contact((PurpleBuddy *) data);
            break;
        case PURPLE_BLIST_CONTACT_NODE:
        case PURPLE_BLIST_GROUP_NODE:
            /* code handles either case */
            node = data;
            break;
        case PURPLE_BLIST_CHAT_NODE:
        case PURPLE_BLIST_OTHER_NODE:
        default:
            /* Not applicable */
            return;
    }

    list = purple_request_fields_get_field(fields, CONTROL_NAME);
    if (ui_ops != NULL && ui_ops->get_timezone_menu_selection != NULL)
    {
        const char *seldata = ui_ops->get_timezone_menu_selection(list->ui_data);
        if (seldata == NULL)
            purple_blist_node_remove_setting(node, SETTING_NAME);
        else
            purple_blist_node_set_string(node, SETTING_NAME, seldata);
    }
    else
    {
        const GList *sellist;
        void *seldata = NULL;
        sellist = purple_request_field_list_get_selected(list);
        if (sellist)
            seldata = purple_request_field_list_get_data(list, sellist->data);

        /* Otherwise, it's fixed value and this means deletion */
        if (seldata == TIMEZONE_FLAG)
            purple_blist_node_set_string(node, SETTING_NAME, sellist->data);
        else if (seldata == DISABLED_FLAG)
            purple_blist_node_set_string(node, SETTING_NAME, "none");
        else
            purple_blist_node_remove_setting(node, SETTING_NAME);
	}
}

static int
buddy_add_timezone_cb(char *filename, void *data)
{
    PurpleRequestField *field = (PurpleRequestField *) data;
    if (isupper(filename[0]))
        purple_request_field_list_add(field, filename, TIMEZONE_FLAG);
    return 0;
}

static void
buddytimezone_createfields_cb(PurpleRequestFields * fields, PurpleBlistNode * data)
{
    purple_debug(PURPLE_DEBUG_INFO, PLUGIN_STATIC_NAME, "buddytimezone_createfields_cb(%p,%p)\n", fields, data);
    PurpleRequestField *field;
    PurpleRequestFieldGroup *group;
    const char *timezone;
    gboolean is_default;

    switch (data->type)
    {
        case PURPLE_BLIST_BUDDY_NODE:
        case PURPLE_BLIST_CONTACT_NODE:
            is_default = FALSE;
            break;
        case PURPLE_BLIST_GROUP_NODE:
            is_default = TRUE;
            break;
        case PURPLE_BLIST_CHAT_NODE:
        case PURPLE_BLIST_OTHER_NODE:
        default:
            /* Not applicable */
            return;
    }

    group = purple_request_field_group_new(NULL);
    purple_request_fields_add_group(fields, group);

    timezone = buddy_get_timezone(data, FALSE, NULL);

    if (ui_ops != NULL && ui_ops->create_menu)
    {
        field =
            purple_request_field_new(CONTROL_NAME,
                                   is_default ? "Default timezone for group" : "Timezone of contact",
                                   PURPLE_REQUEST_FIELD_LIST);
        field->ui_data = ui_ops->create_menu(timezone);
    }
    else
    {
        field =
            purple_request_field_list_new(CONTROL_NAME,
                                        is_default ? "Default timezone for group" :
                                        "Timezone of contact (type to select)");
        purple_request_field_list_set_multi_select(field, FALSE);
        purple_request_field_list_add(field, "<Default>", "");
        purple_request_field_list_add(field, "<Disabled>", DISABLED_FLAG);

        recurse_directory("/usr/share/zoneinfo/", buddy_add_timezone_cb, field);

        if (timezone)
        {
            if (strcmp(timezone, "none") == 0)
                purple_request_field_list_add_selected(field, "<Disabled>");
            else
                purple_request_field_list_add_selected(field, timezone);
        }
        else
            purple_request_field_list_add_selected(field, "<Default>");
    }

    purple_request_field_group_add_field(group, field);
}
#endif

static void
marshal_POINTER__POINTER_BOOL(PurpleCallback cb, va_list args, void *data,
                              void **return_val)
{
	gpointer ret_val;
	void *arg1 = va_arg(args, void *);
	gboolean arg2 = va_arg(args, gboolean);

	ret_val = ((gpointer (*)(void *, gboolean, void *))cb)(arg1, arg2, data);

	if (return_val != NULL)
		*return_val = ret_val;
}

static void
marshal_POINTER__POINTER_POINTER_POINTER(PurpleCallback cb, va_list args, void *data,
                                                void **return_val)
{
	gpointer ret_val;
	void *arg1 = va_arg(args, void *);
	void *arg2 = va_arg(args, void *);
	void *arg3 = va_arg(args, void *);

	ret_val = ((gpointer (*)(void *, void *, void *, void *))cb)(arg1, arg2, arg3, data);

	if (return_val != NULL)
		*return_val = ret_val;
}

static gboolean
load_ui_plugin(gpointer data)
{
	char *ui_plugin_id;
	PurplePlugin *ui_plugin;

	ui_plugin_id = g_strconcat(purple_core_get_ui(), "-", PLUGIN_STATIC_NAME, NULL);
	ui_plugin = purple_plugins_find_with_id(ui_plugin_id);

	if (ui_plugin != NULL)
	{
		if (!purple_plugin_load(ui_plugin))
		{
			purple_notify_error(ui_plugin, NULL, _("Failed to load the Buddy Timezone UI."),
			                    ui_plugin->error ? ui_plugin->error : "");
		}
	}

	g_free(ui_plugin_id);

	return FALSE;
}

static gboolean
plugin_load(PurplePlugin * plugin)
{
	plugin_self = plugin;

	purple_signal_connect(purple_conversations_get_handle(), "conversation-created", plugin,
	                      PURPLE_CALLBACK(timezone_createconv_cb), NULL);

	purple_plugin_ipc_register(plugin, BUDDYTIME_BUDDY_GET_TIMEZONE,
	                           PURPLE_CALLBACK(buddy_get_timezone),
	                           marshal_POINTER__POINTER_BOOL,
	                           purple_value_new(PURPLE_TYPE_STRING),
	                           2,
	                           purple_value_new(PURPLE_TYPE_SUBTYPE,
	                                            PURPLE_SUBTYPE_BLIST_NODE),
	                           purple_value_new(PURPLE_TYPE_BOOLEAN));

	purple_plugin_ipc_register(plugin, BUDDYTIME_TIMEZONE_GET_TIME,
	                           PURPLE_CALLBACK(timezone_get_time),
	                           marshal_POINTER__POINTER_POINTER_POINTER,
	                           purple_value_new(PURPLE_TYPE_INT),
	                           2,
	                           purple_value_new(PURPLE_TYPE_POINTER),
	                           purple_value_new(PURPLE_TYPE_POINTER));

	/* This is done as an idle callback to avoid an infinite loop
	 * when we try to load the UI plugin which depends on this plugin
	 * which isn't officially loaded yet. */
	purple_timeout_add(0, load_ui_plugin, NULL);

	return TRUE;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	0,
	PURPLE_PLUGIN_STANDARD,          /**< type           */
	NULL,                            /**< ui_requirement */
	0,                               /**< flags          */
	NULL,                            /**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,         /**< priority       */
	PLUGIN_ID,                       /**< id             */
	NULL,                            /**< name           */
	PP_VERSION,                      /**< version        */
	NULL,                            /**< summary        */
	NULL,                            /**< description    */
	PLUGIN_AUTHOR,	                 /**< author         */
	PP_WEBSITE,                      /**< homepage       */
	plugin_load,                     /**< load           */
	NULL,                            /**< unload         */
	NULL,                            /**< destroy        */
	NULL,                            /**< ui_info        */
	NULL,                            /**< extra_info     */
	NULL,                            /**< prefs_info     */
	NULL,                            /**< actions        */
	NULL,                            /**< reserved 1     */
	NULL,                            /**< reserved 2     */
	NULL,                            /**< reserved 3     */
	NULL                             /**< reserved 4     */
};

static void
init_plugin(PurplePlugin * plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Buddy Time");
	info.summary = _("Quickly see the local time of a buddy");
	info.description = _("Quickly see the local time of a buddy");
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info);

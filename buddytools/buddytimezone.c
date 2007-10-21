/*************************************************************************
 * Buddy Timezone Module
 *
 * A Purple plugin that allows you to configure a timezone on a per-contact
 * basis so it can display the localtime of your contact when a conversation
 * starts. Convenient if you deal with contacts from many parts of the
 * world.
 *
 * by Martijn van Oosterhout <kleptog@svana.org> (C) April 2006
 * Some code copyright (C) 2006, Richard Laager <rlaager@users.sf.net>
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
 *************************************************************************/

#define PURPLE_PLUGINS

#ifdef CUSTOM_GTK
#define PLUGIN "gtk-kleptog-buddytimezone"
#else
#define PLUGIN "core-kleptog-buddytimezone"
#endif

#define SETTING_NAME "timezone"
#define CONTROL_NAME PLUGIN "-" SETTING_NAME

#include <glib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "notify.h"
#include "plugin.h"
#include "version.h"

#include "debug.h"              /* Debug output functions */
#include "util.h"               /* Menu functions */
#include "request.h"            /* Requests stuff */
#include "conversation.h"       /* Conversation stuff */

#include "localtime.h"
#include "recurse.h"

#include "gtkblist.h"

#define TIMEZONE_FLAG  ((void*)1)
#define DISABLED_FLAG ((void*)2)

#define TIME_FORMAT "%X"
/* Another possible format (hides seconds) */
//#define TIME_FORMAT  "%H:%M"

static PurplePlugin *plugin_self;
void *make_timezone_menu(const char *selected);
const char *get_timezone_menu_selection(void *widget);

/* Resolve specifies what the return value should mean:
 *
 * If TRUE, it's for display, we want to know the *effect* thus hiding the
 * "none" value and going to going level to find the default
 *
 * If false, we only want what the user enter, thus the string "none" if
 * that's what it is
 */
static const char *
buddy_get_timezone(PurpleBlistNode * node, gboolean resolve)
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

    if(!resolve)
        return timezone;

    /* The effect of "none" is to stop recursion */
    if(timezone && strcmp(timezone, "none") == 0)
        return NULL;

    if(timezone)
        return timezone;

    if(datanode->type == PURPLE_BLIST_CONTACT_NODE)
    {
        /* There is no purple_blist_contact_get_group(), though there probably should be */
        datanode = datanode->parent;
        timezone = purple_blist_node_get_string(datanode, SETTING_NAME);
    }

    if(timezone && strcmp(timezone, "none") == 0)
        return NULL;

    return timezone;
}

/* Calcuates the difference between two struct tm's. */
static float
timezone_calc_difference(struct tm *remote_tm, struct tm *local_tm)
{
    int datediff = 0, diff;

    /* Tries to calculate the difference. Note this only works because the
     * times are with 24 hours of eachother! */
    if(remote_tm->tm_mday != local_tm->tm_mday)
    {
        datediff = remote_tm->tm_year - local_tm->tm_year;
        if(datediff == 0)
            datediff = remote_tm->tm_mon - local_tm->tm_mon;
        if(datediff == 0)
            datediff = remote_tm->tm_mday - local_tm->tm_mday;
    }
    diff =
        datediff * 24 * 60 + (remote_tm->tm_hour - local_tm->tm_hour) * 60 + (remote_tm->tm_min -
                                                                              local_tm->tm_min);

    return (float)diff / 60.0;
}

#ifdef PRIVATE_TZLIB
static int
timezone_get_time(const char *timezone, struct tm *tm, float *diff)
{
    struct state *tzinfo = timezone_load(timezone);
    struct tm *local_tm;
    time_t now;

    if(!tzinfo)
        return -1;

    time(&now);
    localsub(&now, 0, tm, tzinfo);
    free(tzinfo);

    /* Calculate user's localtime, and compare. If same, no output */
    local_tm = localtime(&now);

    if(local_tm->tm_hour == tm->tm_hour && local_tm->tm_min == tm->tm_min)
        return 1;

    *diff = timezone_calc_difference(tm, local_tm);
    return 0;
}
#else
static int
timezone_get_time(const char *timezone, struct tm *tm, float *diff)
{
    time_t now;
    struct tm *tm_tmp;
    const gchar *old_tz;

    /* Store the current TZ value. */
    old_tz = g_getenv("TZ");

    g_setenv("TZ", timezone, TRUE);

    time(&now);
    tm_tmp = localtime(&now);
    *tm = *tm_tmp;              /* Must copy, localtime uses local buffer */

    /* Reset the old TZ value. */
    if(old_tz == NULL)
        g_unsetenv("TZ");
    else
        g_setenv("TZ", old_tz, TRUE);

    /* Calculate user's localtime, and compare. If same, no output */
    tm_tmp = localtime(&now);

    if(tm_tmp->tm_hour == tm->tm_hour && tm_tmp->tm_min == tm->tm_min)
        return 1;

    *diff = timezone_calc_difference(tm, tm_tmp);
    return 0;
}
#endif

static void
timezone_createconv_cb(PurpleConversation * conv, void *data)
{
    const char *name;
    PurpleBuddy *buddy;
    struct tm tm;
    const char *timezone;
    float diff;
    int ret;

    if(purple_conversation_get_type(conv) != PURPLE_CONV_TYPE_IM)
        return;

    name = purple_conversation_get_name(conv);
    buddy = purple_find_buddy(purple_conversation_get_account(conv), name);
    if(!buddy)
        return;

    timezone = buddy_get_timezone((PurpleBlistNode *) buddy, TRUE);

    if(!timezone)
        return;

    ret = timezone_get_time(timezone, &tm, &diff);

    if(ret == 0)
    {
        const char *text = purple_time_format(&tm);

        char *str = g_strdup_printf("Remote Local Time: %s (%g hours %s)", text, fabs(diff),
                                    (diff < 0) ? "behind" : "ahead");

        purple_conversation_write(conv, PLUGIN, str, PURPLE_MESSAGE_SYSTEM, time(NULL));

        g_free(str);
    }
}

static void
buddytimezone_tooltip_cb(PurpleBlistNode * node, char **text, gboolean full, void *data)
{
    char *newtext;
    const char *timezone;
    struct tm tm;
    float diff;
    int ret;

    if(!full)
        return;

    purple_debug(PURPLE_DEBUG_INFO, PLUGIN, "type %d\n", node->type);
    timezone = buddy_get_timezone(node, TRUE);
    if(!timezone)
        return;

    ret = timezone_get_time(timezone, &tm, &diff);
    if(ret < 0)
        newtext = g_strdup_printf("%s\n<b>Timezone:</b> %s (error)", *text, timezone);
    else if(ret == 0)
    {
        const char *timetext = purple_time_format(&tm);

        newtext =
            g_strdup_printf("%s\n<b>Local Time:</b> %s (%g hours %s)", *text, timetext, fabs(diff),
                            (diff < 0) ? "behind" : "ahead");
    }
    else
        return;

    g_free(*text);
    *text = newtext;
}

static void
buddytimezone_submitfields_cb(PurpleRequestFields * fields, PurpleBlistNode * data)
{
    PurpleBlistNode *node;
    PurpleRequestField *list;

    /* timezone stuff */
    purple_debug(PURPLE_DEBUG_INFO, PLUGIN, "buddytimezone_submitfields_cb(%p,%p)\n", fields, data);

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
#ifdef CUSTOM_GTK
    const char *seldata = get_timezone_menu_selection(list->ui_data);
    if(seldata == NULL)
        purple_blist_node_remove_setting(node, SETTING_NAME);
    else
        purple_blist_node_set_string(node, SETTING_NAME, seldata);
#else
    const GList *sellist;
    void *seldata = NULL;
    sellist = purple_request_field_list_get_selected(list);
    if(sellist)
        seldata = purple_request_field_list_get_data(list, sellist->data);

    /* Otherwise, it's fixed value and this means deletion */
    if(seldata == TIMEZONE_FLAG)
        purple_blist_node_set_string(node, SETTING_NAME, sellist->data);
    else if(seldata == DISABLED_FLAG)
        purple_blist_node_set_string(node, SETTING_NAME, "none");
    else
        purple_blist_node_remove_setting(node, SETTING_NAME);
#endif
}

#ifndef CUSTOM_GTK
static int
buddy_add_timezone_cb(char *filename, void *data)
{
    PurpleRequestField *field = (PurpleRequestField *) data;
    if(isupper(filename[0]))
        purple_request_field_list_add(field, filename, TIMEZONE_FLAG);
    return 0;
}
#endif

static void
buddytimezone_createfields_cb(PurpleRequestFields * fields, PurpleBlistNode * data)
{
    purple_debug(PURPLE_DEBUG_INFO, PLUGIN, "buddytimezone_createfields_cb(%p,%p)\n", fields, data);
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

    timezone = buddy_get_timezone(data, FALSE);

#ifdef CUSTOM_GTK
    field =
        purple_request_field_new(CONTROL_NAME,
                               is_default ? "Default timezone for group" : "Timezone of contact",
                               PURPLE_REQUEST_FIELD_LIST);
    field->ui_data = make_timezone_menu(timezone);
#else
    field =
        purple_request_field_list_new(CONTROL_NAME,
                                    is_default ? "Default timezone for group" :
                                    "Timezone of contact (type to select)");
    purple_request_field_list_set_multi_select(field, FALSE);
    purple_request_field_list_add(field, "<Default>", "");
    purple_request_field_list_add(field, "<Disabled>", DISABLED_FLAG);

    recurse_directory("/usr/share/zoneinfo/", buddy_add_timezone_cb, field);

    if(timezone)
    {
        if(strcmp(timezone, "none") == 0)
            purple_request_field_list_add_selected(field, "<Disabled>");
        else
            purple_request_field_list_add_selected(field, timezone);
    }
    else
        purple_request_field_list_add_selected(field, "<Default>");
#endif

    purple_request_field_group_add_field(group, field);
}

static gboolean
plugin_load(PurplePlugin * plugin)
{
    plugin_self = plugin;

    purple_signal_connect(purple_blist_get_handle(), "core-kleptog-buddyedit-create-fields", plugin,
                        PURPLE_CALLBACK(buddytimezone_createfields_cb), NULL);
    purple_signal_connect(purple_blist_get_handle(), "core-kleptog-buddyedit-submit-fields", plugin,
                        PURPLE_CALLBACK(buddytimezone_submitfields_cb), NULL);
    purple_signal_connect(pidgin_blist_get_handle(), "drawing-tooltip", plugin,
                        PURPLE_CALLBACK(buddytimezone_tooltip_cb), NULL);
    purple_signal_connect(purple_conversations_get_handle(), "conversation-created", plugin,
                        PURPLE_CALLBACK(timezone_createconv_cb), NULL);

#ifdef PRIVATE_TZLIB
    const char *zoneinfo_dir = purple_prefs_get_string("/plugins/timezone/zoneinfo_dir");
    if(tz_init(zoneinfo_dir) < 0)
        purple_debug_error(PLUGIN, "Problem opening zoneinfo dir (%s): %s\n", zoneinfo_dir,
                         strerror(errno));
#endif

    return TRUE;
}

static PurplePluginInfo info = {
    PURPLE_PLUGIN_MAGIC,
    PURPLE_MAJOR_VERSION,
    0,
    PURPLE_PLUGIN_STANDARD,
    NULL,
    0,
    NULL,
    PURPLE_PRIORITY_DEFAULT,

    PLUGIN,
    "Buddy Timezone Module",
    G_STRINGIFY(PLUGIN_VERSION),

    "Quickly see the local time of a buddy",
    "A Purple plugin that allows you to configure a timezone on a per-contact "
        "basis so it can display the localtime of your contact when a conversation "
        "starts. Convenient if you deal with contacts from many parts of the " "world.",
    "Martijn van Oosterhout <kleptog@svana.org>",
    "http://svana.org/kleptog/purple/",

    plugin_load,
    NULL,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL
};

static void
init_plugin(PurplePlugin * plugin)
{
    info.dependencies = g_list_append(info.dependencies, "core-kleptog-buddyedit");
}

PURPLE_INIT_PLUGIN(buddytimezone, init_plugin, info);

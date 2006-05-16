/*************************************************************************
 * Buddy Timezone Module
 *
 * A Gaim plugin that allows you to configure a timezone on a per-contact
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

#define GAIM_PLUGINS

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

void *gaim_gtk_blist_get_handle();

#if defined(CUSTOM_GTK) && (GAIM_MAJOR_VERSION < 2)
//#error Custom GTK Widget only works in Gaim 2
#undef CUSTOM_GTK
#endif

//#include "gtkblist.h"   /* gaim_gtk_blist_get_handle */  Requires gtk-dev

#define TIMEZONE_FLAG  ((void*)1)
#define DISABLED_FLAG ((void*)2)

#define TIME_FORMAT "%X"
/* Another possible format (hides seconds) */
//#define TIME_FORMAT  "%H:%M"

static GaimPlugin *plugin_self;
void *make_timezone_menu(const char *selected);
const char *get_timezone_menu_selection( void *widget );

/* Resolve specifies what the return value should mean:
 *
 * If TRUE, it's for display, we want to know the *effect* thus hiding the
 * "none" value and going to going level to find the default
 *
 * If false, we only want what the user enter, thus the string "none" if
 * that's what it is
 */
static const char *
buddy_get_timezone(GaimBlistNode * node, gboolean resolve)
{
    GaimBlistNode *datanode = NULL;
    const char *timezone;

    switch (node->type)
    {
        case GAIM_BLIST_BUDDY_NODE:
            datanode = (GaimBlistNode *) gaim_buddy_get_contact((GaimBuddy *) node);
            break;
        case GAIM_BLIST_CONTACT_NODE:
            datanode = node;
            break;
        case GAIM_BLIST_GROUP_NODE:
            datanode = node;
            break;
        default:
            return NULL;
    }

    timezone = gaim_blist_node_get_string(datanode, SETTING_NAME);

    if(!resolve)
        return timezone;

    /* The effect of "none" is to stop recursion */
    if(timezone && strcmp(timezone, "none") == 0)
        return NULL;

    if(timezone)
        return timezone;

    if(datanode->type == GAIM_BLIST_CONTACT_NODE)
    {
        /* There is no gaim_blist_contact_get_group(), though there probably should be */
        datanode = datanode->parent;
        timezone = gaim_blist_node_get_string(datanode, SETTING_NAME);
    }

    if(timezone && strcmp(timezone, "none") == 0)
        return NULL;

    return timezone;
}

/* Calcuates the difference between two struct tm's. */
static float timezone_calc_difference( struct tm *remote_tm, struct tm *local_tm )
{
    int datediff = 0, diff;
    
    /* Tries to calculate the difference. Note this only works because the
     * times are with 24 hours of eachother! */
    if( remote_tm->tm_mday != local_tm->tm_mday )
    {
        datediff = remote_tm->tm_year - local_tm->tm_year;
        if( datediff == 0 )
            datediff = remote_tm->tm_mon - local_tm->tm_mon;
        if( datediff == 0 )
            datediff = remote_tm->tm_mday - local_tm->tm_mday;
    }    
    diff = datediff*24*60 + (remote_tm->tm_hour - local_tm->tm_hour)*60 + (remote_tm->tm_min - local_tm->tm_min);
    
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
    
    if( local_tm->tm_hour == tm->tm_hour && local_tm->tm_min == tm->tm_min )
        return 1;
    
    *diff = timezone_calc_difference( tm, local_tm );
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
    *tm = *tm_tmp;         /* Must copy, localtime uses local buffer */

    /* Reset the old TZ value. */
    if(old_tz == NULL)
        g_unsetenv("TZ");
    else
        g_setenv("TZ", old_tz, TRUE);
        
    /* Calculate user's localtime, and compare. If same, no output */
    tm_tmp = localtime(&now);
    
    if( tm_tmp->tm_hour == tm->tm_hour && tm_tmp->tm_min == tm->tm_min )
        return 1;

    *diff = timezone_calc_difference( tm, tm_tmp );
    return 0;
}
#endif

static void
timezone_createconv_cb(GaimConversation * conv, void *data)
{
    const char *name;
    GaimBuddy *buddy;
    struct tm tm;
    const char *timezone;
    float diff;
    int ret;

#if GAIM_MAJOR_VERSION < 2
    if(gaim_conversation_get_type(conv) != GAIM_CONV_IM)
        return;
#else
    if(gaim_conversation_get_type(conv) != GAIM_CONV_TYPE_IM)
        return;
#endif

    name = gaim_conversation_get_name(conv);
    buddy = gaim_find_buddy(gaim_conversation_get_account(conv), name);
    if(!buddy)
        return;

    timezone = buddy_get_timezone((GaimBlistNode *) buddy, TRUE);

    if(!timezone)
        return;

    ret = timezone_get_time(timezone, &tm, &diff);

    if(ret == 0)
    {
#if GAIM_MAJOR_VERSION > 1
        const char *text = gaim_time_format(&tm);
#else
        char text[64];
        strftime(text, sizeof(text), TIME_FORMAT, &tm);
#endif

        char *str = g_strdup_printf("Remote Local Time: %s (%g hours %s)", text, fabs(diff), (diff<0)?"behind":"ahead");

        gaim_conversation_write(conv, PLUGIN, str, GAIM_MESSAGE_SYSTEM, time(NULL));

        g_free(str);
    }
}

#if GAIM_MAJOR_VERSION > 1
static void
buddytimezone_tooltip_cb(GaimBlistNode * node, char **text, gboolean full, void *data)
#else
static void
buddytimezone_tooltip_cb(GaimBlistNode * node, char **text, void *data)
#endif
{
    char *newtext;
    const char *timezone;
    struct tm tm;
    float diff;
    int ret;

#if GAIM_MAJOR_VERSION > 1
    if(!full)
        return;
#endif

    gaim_debug(GAIM_DEBUG_INFO, PLUGIN, "type %d\n", node->type);
    timezone = buddy_get_timezone(node, TRUE);
    if(!timezone)
        return;

    ret = timezone_get_time(timezone, &tm, &diff);
    if(ret < 0)
        newtext = g_strdup_printf("%s\n<b>Timezone:</b> %s (error)", *text, timezone);
    else if( ret == 0 )
    {
#if GAIM_MAJOR_VERSION > 1
        const char *timetext = gaim_time_format(&tm);
#else
        char timetext[64];
        strftime(timetext, sizeof(timetext), TIME_FORMAT, &tm);
#endif

        newtext = g_strdup_printf("%s\n<b>Local Time:</b> %s (%g hours %s)", *text, timetext, fabs(diff), (diff<0)?"behind":"ahead");
    }
    else
    	return;

    g_free(*text);
    *text = newtext;
}

static void
buddytimezone_submitfields_cb(GaimRequestFields * fields, GaimBlistNode * data)
{
    GaimBlistNode *node;
    GaimRequestField *list;

    /* timezone stuff */
    gaim_debug(GAIM_DEBUG_INFO, PLUGIN, "buddytimezone_submitfields_cb(%p,%p)\n", fields, data);

    switch (data->type)
    {
        case GAIM_BLIST_BUDDY_NODE:
            node = (GaimBlistNode *) gaim_buddy_get_contact((GaimBuddy *) data);
            break;
        case GAIM_BLIST_CONTACT_NODE:
        case GAIM_BLIST_GROUP_NODE:
            /* code handles either case */
            node = data;
            break;
        case GAIM_BLIST_CHAT_NODE:
        case GAIM_BLIST_OTHER_NODE:
        default:
            /* Not applicable */
            return;
    }

    list = gaim_request_fields_get_field(fields, CONTROL_NAME);
#ifdef CUSTOM_GTK
    const char *seldata = get_timezone_menu_selection( list->ui_data );
    if( seldata == NULL )
        gaim_blist_node_remove_setting(node, SETTING_NAME);
    else
        gaim_blist_node_set_string(node, SETTING_NAME, seldata);
#else
    const GList *sellist;
    void *seldata = NULL;
    sellist = gaim_request_field_list_get_selected(list);
    if(sellist)
        seldata = gaim_request_field_list_get_data(list, sellist->data);

    /* Otherwise, it's fixed value and this means deletion */
    if(seldata == TIMEZONE_FLAG)
        gaim_blist_node_set_string(node, SETTING_NAME, sellist->data);
    else if(seldata == DISABLED_FLAG)
        gaim_blist_node_set_string(node, SETTING_NAME, "none");
    else
        gaim_blist_node_remove_setting(node, SETTING_NAME);
#endif
}

#ifndef CUSTOM_GTK
static int
buddy_add_timezone_cb(char *filename, void *data)
{
    GaimRequestField *field = (GaimRequestField *) data;
    if(isupper(filename[0]))
        gaim_request_field_list_add(field, filename, TIMEZONE_FLAG);
    return 0;
}
#endif

static void
buddytimezone_createfields_cb(GaimRequestFields * fields, GaimBlistNode * data)
{
    gaim_debug(GAIM_DEBUG_INFO, PLUGIN, "buddytimezone_createfields_cb(%p,%p)\n", fields, data);
    GaimRequestField *field;
    GaimRequestFieldGroup *group;
    const char *timezone;
    gboolean is_default;

    switch (data->type)
    {
        case GAIM_BLIST_BUDDY_NODE:
        case GAIM_BLIST_CONTACT_NODE:
            is_default = FALSE;
            break;
        case GAIM_BLIST_GROUP_NODE:
            is_default = TRUE;
            break;
        case GAIM_BLIST_CHAT_NODE:
        case GAIM_BLIST_OTHER_NODE:
        default:
            /* Not applicable */
            return;
    }

    group = gaim_request_field_group_new(NULL);
    gaim_request_fields_add_group(fields, group);

    timezone = buddy_get_timezone(data, FALSE);

#ifdef CUSTOM_GTK
    field = gaim_request_field_new( CONTROL_NAME, "Timezone", GAIM_REQUEST_FIELD_LIST );
    field->ui_data = make_timezone_menu(timezone);
#else
    field =
        gaim_request_field_list_new(CONTROL_NAME,
                                    is_default ? "Default timezone for group" :
                                    "Timezone of contact (type to select)");
    gaim_request_field_list_set_multi_select(field, FALSE);
    gaim_request_field_list_add(field, "<Default>", "");
    gaim_request_field_list_add(field, "<Disabled>", DISABLED_FLAG);

    recurse_directory("/usr/share/zoneinfo/", buddy_add_timezone_cb, field);

    if(timezone)
    {
        if(strcmp(timezone, "none") == 0)
            gaim_request_field_list_add_selected(field, "<Disabled>");
        else
            gaim_request_field_list_add_selected(field, timezone);
    }
    else
        gaim_request_field_list_add_selected(field, "<Default>");
#endif

    gaim_request_field_group_add_field(group, field);
}

static gboolean
plugin_load(GaimPlugin * plugin)
{
    plugin_self = plugin;

    gaim_signal_connect(gaim_blist_get_handle(), "core-kleptog-buddyedit-create-fields", plugin,
                        GAIM_CALLBACK(buddytimezone_createfields_cb), NULL);
    gaim_signal_connect(gaim_blist_get_handle(), "core-kleptog-buddyedit-submit-fields", plugin,
                        GAIM_CALLBACK(buddytimezone_submitfields_cb), NULL);
    gaim_signal_connect(gaim_gtk_blist_get_handle(), "drawing-tooltip", plugin,
                        GAIM_CALLBACK(buddytimezone_tooltip_cb), NULL);
    gaim_signal_connect(gaim_conversations_get_handle(), "conversation-created", plugin,
                        GAIM_CALLBACK(timezone_createconv_cb), NULL);

#ifdef PRIVATE_TZLIB
    const char *zoneinfo_dir = gaim_prefs_get_string("/plugins/timezone/zoneinfo_dir");
    if(tz_init(zoneinfo_dir) < 0)
        gaim_debug_error(PLUGIN, "Problem opening zoneinfo dir (%s): %s\n", zoneinfo_dir,
                         strerror(errno));
#endif

    return TRUE;
}

static GaimPluginInfo info = {
    GAIM_PLUGIN_MAGIC,
    GAIM_MAJOR_VERSION,
    GAIM_MINOR_VERSION,
    GAIM_PLUGIN_STANDARD,
    NULL,
    0,
    NULL,
    GAIM_PRIORITY_DEFAULT,

    PLUGIN,
    "Buddy Timezone Module",
    G_STRINGIFY(PLUGIN_VERSION),

    "Quickly see the local time of a buddy",
    "A Gaim plugin that allows you to configure a timezone on a per-contact "
        "basis so it can display the localtime of your contact when a conversation "
        "starts. Convenient if you deal with contacts from many parts of the " "world.",
    "Martijn van Oosterhout <kleptog@svana.org>",
    "http://svana.org/kleptog/gaim/",

    plugin_load,
    NULL,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL
};

static void
init_plugin(GaimPlugin * plugin)
{
    info.dependencies = g_list_append(info.dependencies, "core-kleptog-buddyedit");
}

GAIM_INIT_PLUGIN(buddytimezone, init_plugin, info);

/*************************************************************************
 * Buddy Timezone Module
 * by Martijn van Oosterhout <kleptog@svana.org> (C) April 2006
 * Licenced under the GNU General Public Licence version 2.
 *
 * A Gaim plugin that allows you to configure a timezone on a per-contact
 * basis so it can display the localtime of your contact when a conversation
 * starts. Convenient if you deal with contacts from many parts of the
 * world.
 *************************************************************************/

#define GAIM_PLUGINS
#define PLUGIN "core-kleptog-buddytimezone"
#define SETTING_NAME "timezone"
#define CONTROL_NAME PLUGIN "-" SETTING_NAME

#include <glib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

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

//#include "gtkblist.h"   /* gaim_gtk_blist_get_handle */  Requires gtk-dev

#define TIMEZONE_FLAG  ((void*)1)
#define DISABLED_FLAG ((void*)2)

static GaimPlugin *plugin_self;

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

#ifdef PRIVATE_TZLIB
static int
timezone_get_time(const char *timezone, struct tm *tm)
{
    struct state *tzinfo = timezone_load(timezone);
    if(!tzinfo)
        return -1;
    else
    {
        time_t now = time(NULL);
        localsub(&now, 0, tm, tzinfo);
        free(tzinfo);
    }
    return 0;
}
#else
static int
timezone_get_time(const char *timezone, struct tm *tm_in)
{
    time_t now;
    struct tm *tm;
    const gchar *old_tz;

    /* Store the current TZ value. */
    old_tz = g_getenv("TZ");
    g_setenv("TZ", timezone, TRUE);
    now = time(NULL);
    tm = localtime(&now);
    memcpy(tm_in, tm, sizeof(*tm));

    /* Reset the old TZ value. */
    if(old_tz == NULL)
        g_unsetenv("TZ");
    else
        g_setenv("TZ", old_tz, TRUE);
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

    ret = timezone_get_time(timezone, &tm);

    if(ret == 0)
    {
#if GAIM_MAJOR_VERSION > 1
        char *text = gaim_date_format_short(tm);
#else
        char text[64];
        strftime(text, sizeof(text), "%X", &tm);
#endif

        char *str = g_strdup_printf("Remote Local Time: %s", text);

        gaim_conversation_write(conv, PLUGIN, str, GAIM_MESSAGE_SYSTEM, time(NULL));

        g_free(str);
    }
}

#if GAIM_MAJOR_VERSION > 1
static void buddytimezone_tooltip_cb(GaimBlistNode * node, char **text, gboolean full, void *data);
#else
static void
buddytimezone_tooltip_cb(GaimBlistNode * node, char **text, void *data)
#endif
{
    char *newtext;
    const char *timezone;
    struct tm tm;
    int ret;

#if GAIM_MAJOR_VERSION > 1
    if(!full)
        return;
#endif

    gaim_debug(GAIM_DEBUG_INFO, PLUGIN, "type %d\n", node->type);
    timezone = buddy_get_timezone(node, TRUE);
    if(!timezone)
        return;

    ret = timezone_get_time(timezone, &tm);
    if(ret < 0)
        newtext = g_strdup_printf("%s\n<b>Timezone:</b> %s (error)", *text, timezone);
    else
        newtext = g_strdup_printf("%s\n<b>Local Time:</b> %d:%02d", *text, tm.tm_hour, tm.tm_min);

    g_free(*text);
    *text = newtext;
}

static void
buddytimezone_submitfields_cb(GaimRequestFields * fields, GaimBlistNode * data)
{
    GaimBlistNode *node;
    GaimRequestField *list;
    const GList *sellist;
    void *seldata = NULL;

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
}

static int
buddy_add_timezone_cb(char *filename, void *data)
{
    GaimRequestField *field = (GaimRequestField *) data;
    if(isupper(filename[0]))
        gaim_request_field_list_add(field, filename, TIMEZONE_FLAG);
    return 0;
}

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

    field =
        gaim_request_field_list_new(CONTROL_NAME,
                                    is_default ? "Default timezone for group" :
                                    "Timezone of contact (type to select)");
    gaim_request_field_list_set_multi_select(field, FALSE);
    gaim_request_field_list_add(field, "<Default>", "");
    gaim_request_field_list_add(field, "<Disabled>", DISABLED_FLAG);

    recurse_directory("/usr/share/zoneinfo/", buddy_add_timezone_cb, field);

    timezone = buddy_get_timezone(data, FALSE);
    if(timezone)
    {
        if(strcmp(timezone, "none") == 0)
            gaim_request_field_list_add_selected(field, "<Disabled>");
        else
            gaim_request_field_list_add_selected(field, timezone);
    }
    else
        gaim_request_field_list_add_selected(field, "<Default>");

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

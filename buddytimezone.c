/*************************************************************************
 * Buddy Timezone Module
 * by Martijn van Oosterhout <kleptog@svana.org> (C) April 2006
 * Licenced under the GNU General Public Licence version 2.
 *
 * A GAIM plugin that allows you to configure a timezone on a per-contact
 * basis so it can display the localtime of your contact when a conversation
 * starts. Convenient if you deal with contacts from many parts of the
 * world.
 *************************************************************************/

#define GAIM_PLUGINS
#define PLUGIN "core-kleptog-buddytimezone"

#include <glib.h>
#include <ctype.h>
#include <errno.h>
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

static GaimPlugin *plugin_self;

static const char *
buddy_get_timezone(GaimBlistNode * node)
{
    GaimContact *contact = NULL;
    switch (node->type)
    {
        case GAIM_BLIST_BUDDY_NODE:
            contact = gaim_buddy_get_contact((GaimBuddy *) node);
            break;
        case GAIM_BLIST_CONTACT_NODE:
            contact = (GaimContact *) node;
            break;
        default:
            return NULL;
    }

    return gaim_blist_node_get_string((GaimBlistNode *) contact, "timezone");
}

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

    timezone = buddy_get_timezone((GaimBlistNode *) buddy);

    if(!timezone)
        return;

    ret = timezone_get_time(timezone, &tm);

    if(ret == 0)
    {
        char *str = g_strdup_printf("Remote Local Time: %d:%02d", tm.tm_hour, tm.tm_min);

        gaim_conversation_write(conv, PLUGIN, str, GAIM_MESSAGE_SYSTEM, time(NULL));

        g_free(str);
    }
}

#if GAIM_MAJOR_VERSION > 1
static void
buddytimezone_tooltip_cb(GaimBlistNode * node, char **text, gboolean full, void *data);
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
    if (!full)
        return;
#endif
            
    gaim_debug(GAIM_DEBUG_INFO, PLUGIN, "type %d\n", node->type);
    timezone = buddy_get_timezone(node);
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
buddyedit_submitfields_cb(GaimRequestFields * fields, GaimBlistNode * data)
{
    GaimContact *contact;
    GaimRequestField *list;
    const GList *sellist;
    int is_timezone;

    /* timezone stuff */
    gaim_debug(GAIM_DEBUG_INFO, PLUGIN, "buddyedit_submitfields_cb(%p,%p)\n", fields,
               data);
    if(GAIM_BLIST_NODE_IS_BUDDY(data))
        contact = gaim_buddy_get_contact((GaimBuddy *) data);
    else                        /* Contact */
        contact = (GaimContact *) data;

    list = gaim_request_fields_get_field(fields, "timezone");
    sellist = gaim_request_field_list_get_selected(list);
    /* Timezones have a data value of NULL, in which case we use the item text */
    is_timezone = (sellist
                   && gaim_request_field_list_get_data(list, sellist->data) == TIMEZONE_FLAG);

    /* Otherwise, it's fixed value and this means deletion */
    if(is_timezone)
        gaim_blist_node_set_string((GaimBlistNode *) contact, "timezone", sellist->data);
    else
        gaim_blist_node_remove_setting((GaimBlistNode *) contact, "timezone");
}

static int
buddy_add_timezone_cb(char *filename, void *data)
{
    GaimRequestField *field = (GaimRequestField *) data;
    if(isupper(filename[0]))
        gaim_request_field_list_add(field, filename, TIMEZONE_FLAG);
    return 0;
}

/* Node is either a contact or a buddy */
static void
buddyedit_createfields_cb(GaimRequestFields * fields, GaimBlistNode * data)
{
    gaim_debug(GAIM_DEBUG_INFO, PLUGIN, "buddyedit_createfields_cb(%p,%p)\n", fields,
               data);
    GaimRequestField *field;
    GaimRequestFieldGroup *group;
    const char *timezone;

    group = gaim_request_field_group_new(NULL);
    gaim_request_fields_add_group(fields, group);

    field = gaim_request_field_list_new("timezone", "Timezone of contact (type to select)");
    gaim_request_field_list_set_multi_select(field, FALSE);
    gaim_request_field_list_add(field, "<Disabled>", "");

    recurse_directory("/usr/share/zoneinfo/", buddy_add_timezone_cb, field);

    timezone = buddy_get_timezone(data);
    if(timezone)
        gaim_request_field_list_add_selected(field, timezone);

    gaim_request_field_group_add_field(group, field);
}

static gboolean
plugin_load(GaimPlugin * plugin)
{

    const char *zoneinfo_dir;

    plugin_self = plugin;

    gaim_signal_connect(gaim_blist_get_handle(), "core-kleptog-buddyedit-create-fields", plugin,
                        GAIM_CALLBACK(buddyedit_createfields_cb), NULL);
    gaim_signal_connect(gaim_blist_get_handle(), "core-kleptog-buddyedit-submit-fields", plugin,
                        GAIM_CALLBACK(buddyedit_submitfields_cb), NULL);
    gaim_signal_connect(gaim_gtk_blist_get_handle(), "drawing-tooltip", plugin,
                        GAIM_CALLBACK(buddytimezone_tooltip_cb), NULL);
    gaim_signal_connect(gaim_conversations_get_handle(), "conversation-created", plugin,
                        GAIM_CALLBACK(timezone_createconv_cb), NULL);

    zoneinfo_dir = gaim_prefs_get_string("/plugins/timezone/zoneinfo_dir");
    if(tz_init(zoneinfo_dir) < 0)
        gaim_debug_error(PLUGIN, "Problem opening zoneinfo dir (%s): %s\n", zoneinfo_dir,
                         strerror(errno));

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
    "A GAIM plugin that allows you to configure a timezone on a per-contact "
    "basis so it can display the localtime of your contact when a conversation "
    "starts. Convenient if you deal with contacts from many parts of the "
    "world.",
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

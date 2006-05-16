/*************************************************************************
 * Buddy Notes Module
 * by Martijn van Oosterhout <kleptog@svana.org> (C) April 2006
 * Licenced under the GNU General Public Licence version 2.
 *
 * A Gaim plugin the allows you to add notes to contacts which will be
 * displayed in the conversation screen as well as the hover tooltip.
 *************************************************************************/

#define GAIM_PLUGINS
#define PLUGIN "core-kleptog-buddynotes"
#define SETTING_NAME "notes"
#define CONTROL_NAME PLUGIN "-" SETTING_NAME

#include <glib.h>

#include "notify.h"
#include "plugin.h"
#include "version.h"

#include "debug.h"              /* Debug output functions */
#include "util.h"               /* Menu functions */
#include "request.h"            /* Requests stuff */
#include "conversation.h"       /* Conversation stuff */

void *gaim_gtk_blist_get_handle();

//#include "gtkblist.h"   /* gaim_gtk_blist_get_handle */  Requires gtk-dev

#define TIMEZONE_FLAG  ((void*)1)

static GaimPlugin *plugin_self;

static const char *
buddy_get_notes(GaimBlistNode * node)
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

    return gaim_blist_node_get_string((GaimBlistNode *) contact, SETTING_NAME);
}

static void
buddynotes_createconv_cb(GaimConversation * conv, void *data)
{
    const char *name;
    GaimBuddy *buddy;
    const char *notes;
    char *str;

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

    notes = buddy_get_notes((GaimBlistNode *) buddy);

    if(!notes)
        return;

    str = g_strdup_printf("Notes: %s", notes);

    gaim_conversation_write(conv, PLUGIN, str, GAIM_MESSAGE_SYSTEM, time(NULL));

    g_free(str);
}

static void
buddy_tooltip_cb(GaimBlistNode * node, char **text, void *data)
{
    char *newtext;
    const char *notes;

    gaim_debug(GAIM_DEBUG_INFO, PLUGIN, "type %d\n", node->type);
    notes = buddy_get_notes(node);
    if(!notes)
        return;

    newtext = g_strdup_printf("%s\n<b>Notes:</b> %s", *text, notes);

    g_free(*text);
    *text = newtext;
}

static void
buddynotes_submitfields_cb(GaimRequestFields * fields, GaimBlistNode * data)
{
    GaimContact *contact;
    const char *notes;

    /* buddynotes stuff */
    gaim_debug(GAIM_DEBUG_INFO, PLUGIN, "buddynotes_submitfields_cb(%p,%p)\n", fields, data);
    switch (data->type)
    {
        case GAIM_BLIST_BUDDY_NODE:
            contact = gaim_buddy_get_contact((GaimBuddy *) data);
            break;
        case GAIM_BLIST_CONTACT_NODE:
            contact = (GaimContact *) data;
            break;
        default:
            /* Not applicable */
            return;
    }

    notes = gaim_request_fields_get_string(fields, CONTROL_NAME);

    /* Otherwise, it's fixed value and this means deletion */
    if(notes && notes[0])
        gaim_blist_node_set_string((GaimBlistNode *) contact, SETTING_NAME, notes);
    else
        gaim_blist_node_remove_setting((GaimBlistNode *) contact, SETTING_NAME);
}

/* Node is either a contact or a buddy */
static void
buddynotes_createfields_cb(GaimRequestFields * fields, GaimBlistNode * data)
{
    gaim_debug(GAIM_DEBUG_INFO, PLUGIN, "buddynotes_createfields_cb(%p,%p)\n", fields, data);
    GaimRequestField *field;
    GaimRequestFieldGroup *group;
    const char *notes;

    switch (data->type)
    {
        case GAIM_BLIST_BUDDY_NODE:
        case GAIM_BLIST_CONTACT_NODE:
            /* Continue, code works for either */
            break;
        default:
            /* Not applicable */
            return;
    }
    group = gaim_request_field_group_new(NULL);
    gaim_request_fields_add_group(fields, group);

    notes = buddy_get_notes(data);

    field = gaim_request_field_string_new(CONTROL_NAME, "Notes", notes, FALSE);

    gaim_request_field_group_add_field(group, field);
}

static gboolean
plugin_load(GaimPlugin * plugin)
{

    plugin_self = plugin;

    gaim_signal_connect(gaim_blist_get_handle(), "core-kleptog-buddyedit-create-fields", plugin,
                        GAIM_CALLBACK(buddynotes_createfields_cb), NULL);
    gaim_signal_connect(gaim_blist_get_handle(), "core-kleptog-buddyedit-submit-fields", plugin,
                        GAIM_CALLBACK(buddynotes_submitfields_cb), NULL);
    gaim_signal_connect(gaim_gtk_blist_get_handle(), "drawing-tooltip", plugin,
                        GAIM_CALLBACK(buddy_tooltip_cb), NULL);
    gaim_signal_connect(gaim_conversations_get_handle(), "conversation-created", plugin,
                        GAIM_CALLBACK(buddynotes_createconv_cb), NULL);

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
    "Buddy Notes Module",
    G_STRINGIFY(PLUGIN_VERSION),

    "Store notes about your buddy",
    "This plugin allows you to set a notes field for each buddy and will display it at various points",
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

GAIM_INIT_PLUGIN(buddynotes, init_plugin, info);

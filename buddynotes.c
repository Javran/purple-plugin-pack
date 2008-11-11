/*************************************************************************
 * Buddy Notes Module
 *
 * A Purple plugin the allows you to add notes to contacts which will be
 * displayed in the conversation screen as well as the hover tooltip.
 *
 * by Martijn van Oosterhout <kleptog@svana.org> (C) April 2006
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

#include "gtkblist.h"

#define TIMEZONE_FLAG  ((void*)1)

static PurplePlugin *plugin_self;

static const char *
buddy_get_notes(PurpleBlistNode * node)
{
    PurpleContact *contact = NULL;
    switch (node->type)
    {
        case PURPLE_BLIST_BUDDY_NODE:
            contact = purple_buddy_get_contact((PurpleBuddy *) node);
            break;
        case PURPLE_BLIST_CONTACT_NODE:
            contact = (PurpleContact *) node;
            break;
        default:
            return NULL;
    }

    return purple_blist_node_get_string((PurpleBlistNode *) contact, SETTING_NAME);
}

static void
buddynotes_createconv_cb(PurpleConversation * conv, void *data)
{
    const char *name;
    PurpleBuddy *buddy;
    const char *notes;
    char *str;

    if(purple_conversation_get_type(conv) != PURPLE_CONV_TYPE_IM)
        return;

    name = purple_conversation_get_name(conv);
    buddy = purple_find_buddy(purple_conversation_get_account(conv), name);
    if(!buddy)
        return;

    notes = buddy_get_notes((PurpleBlistNode *) buddy);

    if(!notes)
        return;

    str = g_strdup_printf("Notes: %s", notes);

    purple_conversation_write(conv, PLUGIN, str, PURPLE_MESSAGE_SYSTEM, time(NULL));

    g_free(str);
}

static void
buddy_tooltip_cb(PurpleBlistNode * node, char **text, void *data)
{
    char *newtext;
    const char *notes;

    purple_debug(PURPLE_DEBUG_INFO, PLUGIN, "type %d\n", node->type);
    notes = buddy_get_notes(node);
    if(!notes)
        return;

    newtext = g_strdup_printf("%s\n<b>Notes:</b> %s", *text, notes);

    g_free(*text);
    *text = newtext;
}

static void
buddynotes_submitfields_cb(PurpleRequestFields * fields, PurpleBlistNode * data)
{
    PurpleContact *contact;
    const char *notes;

    /* buddynotes stuff */
    purple_debug(PURPLE_DEBUG_INFO, PLUGIN, "buddynotes_submitfields_cb(%p,%p)\n", fields, data);
    switch (data->type)
    {
        case PURPLE_BLIST_BUDDY_NODE:
            contact = purple_buddy_get_contact((PurpleBuddy *) data);
            break;
        case PURPLE_BLIST_CONTACT_NODE:
            contact = (PurpleContact *) data;
            break;
        default:
            /* Not applicable */
            return;
    }

    notes = purple_request_fields_get_string(fields, CONTROL_NAME);

    /* Otherwise, it's fixed value and this means deletion */
    if(notes && notes[0])
        purple_blist_node_set_string((PurpleBlistNode *) contact, SETTING_NAME, notes);
    else
        purple_blist_node_remove_setting((PurpleBlistNode *) contact, SETTING_NAME);
}

/* Node is either a contact or a buddy */
static void
buddynotes_createfields_cb(PurpleRequestFields * fields, PurpleBlistNode * data)
{
    purple_debug(PURPLE_DEBUG_INFO, PLUGIN, "buddynotes_createfields_cb(%p,%p)\n", fields, data);
    PurpleRequestField *field;
    PurpleRequestFieldGroup *group;
    const char *notes;

    switch (data->type)
    {
        case PURPLE_BLIST_BUDDY_NODE:
        case PURPLE_BLIST_CONTACT_NODE:
            /* Continue, code works for either */
            break;
        default:
            /* Not applicable */
            return;
    }
    group = purple_request_field_group_new(NULL);
    purple_request_fields_add_group(fields, group);

    notes = buddy_get_notes(data);

    field = purple_request_field_string_new(CONTROL_NAME, "Notes", notes, FALSE);

    purple_request_field_group_add_field(group, field);
}

static gboolean
plugin_load(PurplePlugin * plugin)
{

    plugin_self = plugin;

    purple_signal_connect(purple_blist_get_handle(), "core-kleptog-buddyedit-create-fields", plugin,
                          PURPLE_CALLBACK(buddynotes_createfields_cb), NULL);
    purple_signal_connect(purple_blist_get_handle(), "core-kleptog-buddyedit-submit-fields", plugin,
                          PURPLE_CALLBACK(buddynotes_submitfields_cb), NULL);
    purple_signal_connect(pidgin_blist_get_handle(), "drawing-tooltip", plugin,
                          PURPLE_CALLBACK(buddy_tooltip_cb), NULL);
    purple_signal_connect(purple_conversations_get_handle(), "conversation-created", plugin,
                          PURPLE_CALLBACK(buddynotes_createconv_cb), NULL);

    return TRUE;
}

static PurplePluginInfo info = {
    PURPLE_PLUGIN_MAGIC,
    PURPLE_MAJOR_VERSION,
    PURPLE_MINOR_VERSION,
    PURPLE_PLUGIN_STANDARD,
    NULL,
    0,
    NULL,
    PURPLE_PRIORITY_DEFAULT,

    PLUGIN,
    "Buddy Notes Module",
    G_STRINGIFY(PLUGIN_VERSION),

    "Store notes about your buddy",
    "This plugin allows you to set a notes field for each buddy and will display it at various points",
    "Martijn van Oosterhout <kleptog@svana.org>",
    "http://buddytools.sf.net",

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

PURPLE_INIT_PLUGIN(buddynotes, init_plugin, info);

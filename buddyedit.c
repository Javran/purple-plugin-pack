/*************************************************************************
 * Buddy Edit Module
 *
 * A Gaim plugin that adds an edit to to buddies allowing you to change
 * various details you can't normally change. It also provides a mechanism
 * for subsequent plugins to add themselves to that dialog.
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
#define PLUGIN "core-kleptog-buddyedit"

#include <glib.h>
#include <string.h>

#include "notify.h"
#include "plugin.h"
#include "util.h"
#include "version.h"

#include "debug.h"              /* Debug output functions */
#include "request.h"            /* Requests stuff */

static GaimPlugin *plugin_self;

static void
buddyedit_editcomplete_cb(GaimBlistNode * data, GaimRequestFields * fields)
{
    gboolean buddy_destroy = FALSE;

    /* Account detail stuff */
    switch (data->type)
    {
        case GAIM_BLIST_BUDDY_NODE:
        {
            GaimBuddy *buddy = (GaimBuddy *) data;
            GaimAccount *account = gaim_request_fields_get_account(fields, "account");
            const char *name = gaim_request_fields_get_string(fields, "name");
            const char *alias = gaim_request_fields_get_string(fields, "alias");

            /* If any details changes, create the buddy */
            if((account != buddy->account) || strcmp(name, buddy->name))
            {
                GHashTable *tmp;
                GaimBuddy *newbuddy = gaim_buddy_new(account, name, alias);
                gaim_blist_add_buddy(newbuddy, NULL, NULL, data);       /* Copy it to correct location */

                /* Now this is ugly, but we want to copy the settings and avoid issues with memory management */
                tmp = ((GaimBlistNode *) buddy)->settings;
                ((GaimBlistNode *) buddy)->settings = ((GaimBlistNode *) newbuddy)->settings;
                ((GaimBlistNode *) newbuddy)->settings = tmp;

                buddy_destroy = TRUE;
                data = (GaimBlistNode *) newbuddy;
            }
            else
                gaim_blist_alias_buddy(buddy, alias);
            break;
        }
        case GAIM_BLIST_CONTACT_NODE:
        {
            GaimContact *contact = (GaimContact *) data;
            const char *alias = gaim_request_fields_get_string(fields, "alias");
            gaim_contact_set_alias(contact, alias);
            break;
        }
        case GAIM_BLIST_GROUP_NODE:
        {
            GaimGroup *group = (GaimGroup *) data;
            const char *alias = gaim_request_fields_get_string(fields, "alias");
            gaim_blist_rename_group(group, alias);
            break;
        }
        case GAIM_BLIST_CHAT_NODE:
        {
            GaimChat *chat = (GaimChat *) data;
            const char *alias = gaim_request_fields_get_string(fields, "alias");
            gaim_blist_alias_chat(chat, alias);
            break;
        }
        case GAIM_BLIST_OTHER_NODE:
            break;
    }

    gaim_signal_emit(gaim_blist_get_handle(), PLUGIN "-submit-fields", fields, data);

    if(buddy_destroy)
        gaim_blist_remove_buddy((GaimBuddy *) data);
    /* Save any changes */
    gaim_blist_schedule_save();
}

/* Node is either a contact or a buddy */
static void
buddy_edit_cb(GaimBlistNode * node, gpointer data)
{
    gaim_debug(GAIM_DEBUG_INFO, PLUGIN, "buddy_edit_cb(%p)\n", node);
    GaimRequestFields *fields;
    GaimRequestField *field;
    GaimRequestFieldGroup *group;
    char *request_title = NULL;

    fields = gaim_request_fields_new();

    switch (node->type)
    {
        case GAIM_BLIST_BUDDY_NODE:
        {
            GaimBuddy *buddy = (GaimBuddy *) node;
            group = gaim_request_field_group_new("Buddy Details");
            gaim_request_fields_add_group(fields, group);

            field = gaim_request_field_account_new("account", "Account", buddy->account);
            gaim_request_field_account_set_show_all(field, TRUE);
            gaim_request_field_group_add_field(group, field);

            field = gaim_request_field_string_new("name", "Name", buddy->name, FALSE);
            gaim_request_field_group_add_field(group, field);

            field = gaim_request_field_string_new("alias", "Alias", buddy->alias, FALSE);
            gaim_request_field_group_add_field(group, field);

            request_title = "Edit Buddy";
            break;
        }
        case GAIM_BLIST_CONTACT_NODE:
        {
            GaimContact *contact = (GaimContact *) node;
            group = gaim_request_field_group_new("Contact Details");
            gaim_request_fields_add_group(fields, group);

            field = gaim_request_field_string_new("alias", "Alias", contact->alias, FALSE);
            gaim_request_field_group_add_field(group, field);

            request_title = "Edit Contact";
            break;
        }
        case GAIM_BLIST_GROUP_NODE:
        {
            GaimGroup *grp = (GaimGroup *) node;
            group = gaim_request_field_group_new("Group Details");
            gaim_request_fields_add_group(fields, group);

            field = gaim_request_field_string_new("alias", "Name", grp->name, FALSE);
            gaim_request_field_group_add_field(group, field);

            request_title = "Edit Group";
            break;
        }
        case GAIM_BLIST_CHAT_NODE:
        {
            GaimChat *chat = (GaimChat *) node;
            group = gaim_request_field_group_new("Chat Details");
            gaim_request_fields_add_group(fields, group);

            field = gaim_request_field_string_new("alias", "Alias", chat->alias, FALSE);
            gaim_request_field_group_add_field(group, field);

            request_title = "Edit Chat";
            break;
        }
        default:
            request_title = "Edit";
            break;
    }

    gaim_signal_emit(gaim_blist_get_handle(), PLUGIN "-create-fields", fields, node);

    gaim_request_fields(plugin_self,
                        request_title, NULL, NULL,
                        fields, "OK", G_CALLBACK(buddyedit_editcomplete_cb), "Cancel", NULL, node);
}

static void
buddy_menu_cb(GaimBlistNode * node, GList ** menu, void *data)
{
#if GAIM_MAJOR_VERSION < 2
    GaimBlistNodeAction *action;
#else
    GaimMenuAction *action;
#endif

    switch (node->type)
    {
            /* These are the types we handle */
        case GAIM_BLIST_BUDDY_NODE:
        case GAIM_BLIST_CONTACT_NODE:
        case GAIM_BLIST_GROUP_NODE:
        case GAIM_BLIST_CHAT_NODE:
            break;

        case GAIM_BLIST_OTHER_NODE:
        default:
            return;
    }

#if GAIM_MAJOR_VERSION < 2
    action = gaim_blist_node_action_new("Edit...", buddy_edit_cb, NULL);
#else
    action = gaim_menu_action_new("Edit...", GAIM_CALLBACK(buddy_edit_cb), NULL, NULL);
#endif
    *menu = g_list_append(*menu, action);
}

static gboolean
plugin_load(GaimPlugin * plugin)
{

    plugin_self = plugin;

    void *blist_handle = gaim_blist_get_handle();

    gaim_signal_register(blist_handle, PLUGIN "-create-fields", /* Called when about to create dialog */
                         gaim_marshal_VOID__POINTER_POINTER, NULL, 2,   /* (FieldList*,BlistNode*) */
                         gaim_value_new(GAIM_TYPE_SUBTYPE, GAIM_TYPE_POINTER),  /* FieldList */
                         gaim_value_new(GAIM_TYPE_SUBTYPE, GAIM_SUBTYPE_BLIST));

    gaim_signal_register(blist_handle, PLUGIN "-submit-fields", /* Called when dialog submitted */
                         gaim_marshal_VOID__POINTER_POINTER, NULL, 2,   /* (FieldList*,BlistNode*) */
                         gaim_value_new(GAIM_TYPE_SUBTYPE, GAIM_TYPE_POINTER),  /* FieldList */
                         gaim_value_new(GAIM_TYPE_SUBTYPE, GAIM_SUBTYPE_BLIST));

    gaim_signal_connect(blist_handle, "blist-node-extended-menu", plugin,
                        GAIM_CALLBACK(buddy_menu_cb), NULL);

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
    "Buddy Edit Module",
    G_STRINGIFY(PLUGIN_VERSION),

    "Enable editing of buddy properties",
    "A plugin that adds an edit to to buddies allowing you to change various details you can't normally change. "
        "It also provides a mechanism for subsequent plugins to add themselves to that dialog. ",
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
}

GAIM_INIT_PLUGIN(buddyedit, init_plugin, info);

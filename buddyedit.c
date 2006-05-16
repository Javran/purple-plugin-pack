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
    gboolean blist_destroy = FALSE;

    GaimBlistNode *olddata = data;      /* Keep pointer in case we need to destroy it */

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

                blist_destroy = TRUE;
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
            gboolean new_chat = FALSE;

            GaimConnection *gc;
            GList *list = NULL, *tmp;
            gc = gaim_account_get_connection(chat->account);
            if(GAIM_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info != NULL)
                list = GAIM_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info(gc);

            GaimAccount *newaccount = gaim_request_fields_get_account(fields, "account");

            /* In Gaim2 each prot_chat_entry has a field "required". We use
             * this to determine if a field is important enough to recreate
             * the chat if it changes. Non-required fields we jsut change
             * in-situ. In Gaim1.5 this field doesn't exist so we always
             * recreate */
#if GAIM_MAJOR_VERSION >= 2
            if(newaccount != chat->account)
                new_chat = TRUE;
            else
            {
                const char *oldvalue, *newvalue;

                for (tmp = g_list_first(list); tmp && !new_chat; tmp = g_list_next(tmp))
                {
                    struct proto_chat_entry *pce = tmp->data;
                    if(!pce->required)  /* Only checking required fields at this point */
                        continue;
                    if(pce->is_int)
                        continue;       /* Not yet */
                    oldvalue = g_hash_table_lookup(chat->components, pce->identifier);
                    newvalue = gaim_request_fields_get_string(fields, pce->identifier);

                    if(oldvalue == NULL)
                        oldvalue = "";
                    if(newvalue == NULL)
                        newvalue = "";
                    if(strcmp(oldvalue, newvalue) != 0)
                        new_chat = TRUE;
                }
            }
#else
            new_chat = TRUE;
#endif

            if(new_chat)
            {
                const char *oldvalue, *newvalue;
                GHashTable *components =
                    g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

                for (tmp = g_list_first(list); tmp; tmp = g_list_next(tmp))
                {
                    struct proto_chat_entry *pce = tmp->data;

                    if(pce->is_int)
                    {
                        oldvalue = g_hash_table_lookup(chat->components, pce->identifier);
                        g_hash_table_replace(components, g_strdup(pce->identifier),
                                             g_strdup(oldvalue));
                    }
                    else
                    {
                        newvalue = gaim_request_fields_get_string(fields, pce->identifier);
                        g_hash_table_replace(components, g_strdup(pce->identifier),
                                             g_strdup(newvalue));
                    }
                }
                GaimChat *newchat = gaim_chat_new(newaccount, NULL, components);
                gaim_blist_add_chat(newchat, NULL, data);       /* Copy it to correct location */
                data = (GaimBlistNode *) newchat;
                blist_destroy = TRUE;
            }
            else                /* Just updating values in old chat */
            {
                const char *newvalue;
                for (tmp = g_list_first(list); tmp; tmp = g_list_next(tmp))
                {
                    struct proto_chat_entry *pce = tmp->data;
#if GAIM_MAJOR_VERSION >= 2
                    if(pce->required)
                        continue;
#endif
                    if(pce->is_int)
                    {
                        /* Do nothing, yet */
                    }
                    else
                    {
                        newvalue = gaim_request_fields_get_string(fields, pce->identifier);
                        g_hash_table_replace(chat->components, g_strdup(pce->identifier),
                                             g_strdup(newvalue));
                    }
                }
            }

            const char *alias = gaim_request_fields_get_string(fields, "alias");
            gaim_blist_alias_chat(chat, alias);
            break;
        }
        case GAIM_BLIST_OTHER_NODE:
            break;
    }

    gaim_signal_emit(gaim_blist_get_handle(), PLUGIN "-submit-fields", fields, data);

    if(blist_destroy)
    {
        if(olddata->type == GAIM_BLIST_BUDDY_NODE)
            gaim_blist_remove_buddy((GaimBuddy *) olddata);
        else if(olddata->type == GAIM_BLIST_CHAT_NODE)
            gaim_blist_remove_chat((GaimChat *) olddata);
    }

    /* Save any changes */
    gaim_blist_schedule_save();
}

static GaimAccount *buddyedit_account_filter_func_data;

static gboolean
buddyedit_account_filter_func(GaimAccount * account)
{
    GaimPluginProtocolInfo *gppi1 =
        GAIM_PLUGIN_PROTOCOL_INFO(gaim_account_get_connection(account)->prpl);
    GaimPluginProtocolInfo *gppi2 =
        GAIM_PLUGIN_PROTOCOL_INFO(gaim_account_get_connection(buddyedit_account_filter_func_data)->
                                  prpl);

    return gppi1 == gppi2;
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

            field = gaim_request_field_account_new("account", "Account", chat->account);
            gaim_request_field_account_set_filter(field, buddyedit_account_filter_func);
            buddyedit_account_filter_func_data = chat->account;
            gaim_request_field_group_add_field(group, field);
            GaimConnection *gc;
            GList *list = NULL, *tmp;
            gc = gaim_account_get_connection(chat->account);
            if(GAIM_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info != NULL)
                list = GAIM_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info(gc);

            for (tmp = g_list_first(list); tmp; tmp = g_list_next(tmp))
            {
                struct proto_chat_entry *pce = tmp->data;
                const char *value;
#if GAIM_MAJOR_VERSION >= 2
                gaim_debug(GAIM_DEBUG_INFO, PLUGIN,
                           "identifier=%s, label=%s, is_int=%d, required=%d\n", pce->identifier,
                           pce->label, pce->is_int, pce->required);
#endif
                if(pce->is_int)
                    continue;   /* Not yet */
                value = g_hash_table_lookup(chat->components, pce->identifier);
                field = gaim_request_field_string_new(pce->identifier, pce->label, value, FALSE);
#if GAIM_MAJOR_VERSION >= 2
                gaim_request_field_set_required(field, pce->required);
#endif
                gaim_request_field_group_add_field(group, field);
            }
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

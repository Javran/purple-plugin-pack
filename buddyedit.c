/*************************************************************************
 * Buddy Edit Module
 *
 * A Purple plugin that adds an edit to to buddies allowing you to change
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

#define PURPLE_PLUGINS
#define PLUGIN "core-kleptog-buddyedit"

#include <glib.h>
#include <string.h>

#include "notify.h"
#include "plugin.h"
#include "util.h"
#include "version.h"

#include "debug.h"              /* Debug output functions */
#include "request.h"            /* Requests stuff */

static PurplePlugin *plugin_self;

static void
buddyedit_editcomplete_cb(PurpleBlistNode * data, PurpleRequestFields * fields)
{
    gboolean blist_destroy = FALSE;

    PurpleBlistNode *olddata = data;      /* Keep pointer in case we need to destroy it */

    /* Account detail stuff */
    switch (data->type)
    {
        case PURPLE_BLIST_BUDDY_NODE:
        {
            PurpleBuddy *buddy = (PurpleBuddy *) data;
            PurpleAccount *account = purple_request_fields_get_account(fields, "account");
            const char *name = purple_request_fields_get_string(fields, "name");
            const char *alias = purple_request_fields_get_string(fields, "alias");

            /* If any details changes, create the buddy */
            if((account != buddy->account) || strcmp(name, buddy->name))
            {
                GHashTable *tmp;
                PurpleBuddy *newbuddy = purple_buddy_new(account, name, alias);
                purple_blist_add_buddy(newbuddy, NULL, NULL, data);       /* Copy it to correct location */

                /* Now this is ugly, but we want to copy the settings and avoid issues with memory management */
                tmp = ((PurpleBlistNode *) buddy)->settings;
                ((PurpleBlistNode *) buddy)->settings = ((PurpleBlistNode *) newbuddy)->settings;
                ((PurpleBlistNode *) newbuddy)->settings = tmp;

                blist_destroy = TRUE;
                data = (PurpleBlistNode *) newbuddy;
            }
            else
                purple_blist_alias_buddy(buddy, alias);
            break;
        }
        case PURPLE_BLIST_CONTACT_NODE:
        {
            PurpleContact *contact = (PurpleContact *) data;
            const char *alias = purple_request_fields_get_string(fields, "alias");
            purple_contact_set_alias(contact, alias);
            break;
        }
        case PURPLE_BLIST_GROUP_NODE:
        {
            PurpleGroup *group = (PurpleGroup *) data;
            const char *alias = purple_request_fields_get_string(fields, "alias");
            purple_blist_rename_group(group, alias);
            break;
        }
        case PURPLE_BLIST_CHAT_NODE:
        {
            PurpleChat *chat = (PurpleChat *) data;
            gboolean new_chat = FALSE;

            PurpleConnection *gc;
            GList *list = NULL, *tmp;
            gc = purple_account_get_connection(chat->account);
            if(PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info != NULL)
                list = PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info(gc);

            PurpleAccount *newaccount = purple_request_fields_get_account(fields, "account");

            /* In Purple2 each prot_chat_entry has a field "required". We use
             * this to determine if a field is important enough to recreate
             * the chat if it changes. Non-required fields we jsut change
             * in-situ. In Purple1.5 this field doesn't exist so we always
             * recreate */
#if PURPLE_MAJOR_VERSION >= 2
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
                    newvalue = purple_request_fields_get_string(fields, pce->identifier);

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
                        newvalue = purple_request_fields_get_string(fields, pce->identifier);
                        g_hash_table_replace(components, g_strdup(pce->identifier),
                                             g_strdup(newvalue));
                    }
                }
                PurpleChat *newchat = purple_chat_new(newaccount, NULL, components);
                purple_blist_add_chat(newchat, NULL, data);       /* Copy it to correct location */
                data = (PurpleBlistNode *) newchat;
                blist_destroy = TRUE;
            }
            else                /* Just updating values in old chat */
            {
                const char *newvalue;
                for (tmp = g_list_first(list); tmp; tmp = g_list_next(tmp))
                {
                    struct proto_chat_entry *pce = tmp->data;
#if PURPLE_MAJOR_VERSION >= 2
                    if(pce->required)
                        continue;
#endif
                    if(pce->is_int)
                    {
                        /* Do nothing, yet */
                    }
                    else
                    {
                        newvalue = purple_request_fields_get_string(fields, pce->identifier);
                        g_hash_table_replace(chat->components, g_strdup(pce->identifier),
                                             g_strdup(newvalue));
                    }
                }
            }

            const char *alias = purple_request_fields_get_string(fields, "alias");
            purple_blist_alias_chat(chat, alias);
            break;
        }
        case PURPLE_BLIST_OTHER_NODE:
            break;
    }

    purple_signal_emit(purple_blist_get_handle(), PLUGIN "-submit-fields", fields, data);

    if(blist_destroy)
    {
        if(olddata->type == PURPLE_BLIST_BUDDY_NODE)
            purple_blist_remove_buddy((PurpleBuddy *) olddata);
        else if(olddata->type == PURPLE_BLIST_CHAT_NODE)
            purple_blist_remove_chat((PurpleChat *) olddata);
    }

    /* Save any changes */
    purple_blist_schedule_save();
}

static PurpleAccount *buddyedit_account_filter_func_data;

static gboolean
buddyedit_account_filter_func(PurpleAccount * account)
{
    PurplePluginProtocolInfo *gppi1 =
        PURPLE_PLUGIN_PROTOCOL_INFO(purple_account_get_connection(account)->prpl);
    PurplePluginProtocolInfo *gppi2 =
        PURPLE_PLUGIN_PROTOCOL_INFO(purple_account_get_connection(buddyedit_account_filter_func_data)->
                                  prpl);

    return gppi1 == gppi2;
}

/* Node is either a contact or a buddy */
static void
buddy_edit_cb(PurpleBlistNode * node, gpointer data)
{
    purple_debug(PURPLE_DEBUG_INFO, PLUGIN, "buddy_edit_cb(%p)\n", node);
    PurpleRequestFields *fields;
    PurpleRequestField *field;
    PurpleRequestFieldGroup *group;
    char *request_title = NULL;

    fields = purple_request_fields_new();

    switch (node->type)
    {
        case PURPLE_BLIST_BUDDY_NODE:
        {
            PurpleBuddy *buddy = (PurpleBuddy *) node;
            group = purple_request_field_group_new("Buddy Details");
            purple_request_fields_add_group(fields, group);

            field = purple_request_field_account_new("account", "Account", buddy->account);
            purple_request_field_account_set_show_all(field, TRUE);
            purple_request_field_group_add_field(group, field);

            field = purple_request_field_string_new("name", "Name", buddy->name, FALSE);
            purple_request_field_group_add_field(group, field);

            field = purple_request_field_string_new("alias", "Alias", buddy->alias, FALSE);
            purple_request_field_group_add_field(group, field);

            request_title = "Edit Buddy";
            break;
        }
        case PURPLE_BLIST_CONTACT_NODE:
        {
            PurpleContact *contact = (PurpleContact *) node;
            group = purple_request_field_group_new("Contact Details");
            purple_request_fields_add_group(fields, group);

            field = purple_request_field_string_new("alias", "Alias", contact->alias, FALSE);
            purple_request_field_group_add_field(group, field);

            request_title = "Edit Contact";
            break;
        }
        case PURPLE_BLIST_GROUP_NODE:
        {
            PurpleGroup *grp = (PurpleGroup *) node;
            group = purple_request_field_group_new("Group Details");
            purple_request_fields_add_group(fields, group);

            field = purple_request_field_string_new("alias", "Name", grp->name, FALSE);
            purple_request_field_group_add_field(group, field);

            request_title = "Edit Group";
            break;
        }
        case PURPLE_BLIST_CHAT_NODE:
        {
            PurpleChat *chat = (PurpleChat *) node;
            PurpleConnection *gc;
            GList *list = NULL, *tmp;
            group = purple_request_field_group_new("Chat Details");
            purple_request_fields_add_group(fields, group);

            field = purple_request_field_account_new("account", "Account", chat->account);
            purple_request_field_account_set_filter(field, buddyedit_account_filter_func);
            buddyedit_account_filter_func_data = chat->account;
            purple_request_field_group_add_field(group, field);
            gc = purple_account_get_connection(chat->account);
            if(PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info != NULL)
                list = PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info(gc);

            for (tmp = g_list_first(list); tmp; tmp = g_list_next(tmp))
            {
                struct proto_chat_entry *pce = tmp->data;
                const char *value;
#if PURPLE_MAJOR_VERSION >= 2
                purple_debug(PURPLE_DEBUG_INFO, PLUGIN,
                           "identifier=%s, label=%s, is_int=%d, required=%d\n", pce->identifier,
                           pce->label, pce->is_int, pce->required);
#endif
                if(pce->is_int)
                    continue;   /* Not yet */
                value = g_hash_table_lookup(chat->components, pce->identifier);
                field = purple_request_field_string_new(pce->identifier, pce->label, value, FALSE);
#if PURPLE_MAJOR_VERSION >= 2
                purple_request_field_set_required(field, pce->required);
#endif
                purple_request_field_group_add_field(group, field);
            }
            field = purple_request_field_string_new("alias", "Alias", chat->alias, FALSE);
            purple_request_field_group_add_field(group, field);

            request_title = "Edit Chat";
            break;
        }
        default:
            request_title = "Edit";
            break;
    }

    purple_signal_emit(purple_blist_get_handle(), PLUGIN "-create-fields", fields, node);

    purple_request_fields(plugin_self, request_title, NULL, NULL, fields,
                          "OK", G_CALLBACK(buddyedit_editcomplete_cb),
                          "Cancel", NULL,
                          NULL, NULL, NULL, /* XXX: These should be set. */
                          node);
}

static void
buddy_menu_cb(PurpleBlistNode * node, GList ** menu, void *data)
{
#if PURPLE_MAJOR_VERSION < 2
    PurpleBlistNodeAction *action;
#else
    PurpleMenuAction *action;
#endif

    switch (node->type)
    {
            /* These are the types we handle */
        case PURPLE_BLIST_BUDDY_NODE:
        case PURPLE_BLIST_CONTACT_NODE:
        case PURPLE_BLIST_GROUP_NODE:
        case PURPLE_BLIST_CHAT_NODE:
            break;

        case PURPLE_BLIST_OTHER_NODE:
        default:
            return;
    }

#if PURPLE_MAJOR_VERSION < 2
    action = purple_blist_node_action_new("Edit...", buddy_edit_cb, NULL);
#else
    action = purple_menu_action_new("Edit...", PURPLE_CALLBACK(buddy_edit_cb), NULL, NULL);
#endif
    *menu = g_list_append(*menu, action);
}

static gboolean
plugin_load(PurplePlugin * plugin)
{

    plugin_self = plugin;

    void *blist_handle = purple_blist_get_handle();

    purple_signal_register(blist_handle, PLUGIN "-create-fields", /* Called when about to create dialog */
                         purple_marshal_VOID__POINTER_POINTER, NULL, 2,   /* (FieldList*,BlistNode*) */
                         purple_value_new(PURPLE_TYPE_SUBTYPE, PURPLE_TYPE_POINTER),  /* FieldList */
                         purple_value_new(PURPLE_TYPE_SUBTYPE, PURPLE_SUBTYPE_BLIST));

    purple_signal_register(blist_handle, PLUGIN "-submit-fields", /* Called when dialog submitted */
                         purple_marshal_VOID__POINTER_POINTER, NULL, 2,   /* (FieldList*,BlistNode*) */
                         purple_value_new(PURPLE_TYPE_SUBTYPE, PURPLE_TYPE_POINTER),  /* FieldList */
                         purple_value_new(PURPLE_TYPE_SUBTYPE, PURPLE_SUBTYPE_BLIST));

    purple_signal_connect(blist_handle, "blist-node-extended-menu", plugin,
                        PURPLE_CALLBACK(buddy_menu_cb), NULL);

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
    "Buddy Edit Module",
    G_STRINGIFY(PLUGIN_VERSION),

    "Enable editing of buddy properties",
    "A plugin that adds an edit to to buddies allowing you to change various details you can't normally change. "
        "It also provides a mechanism for subsequent plugins to add themselves to that dialog. ",
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
}

PURPLE_INIT_PLUGIN(buddyedit, init_plugin, info);

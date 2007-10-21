/*************************************************************************
 * Buddy Language Module
 *
 * A Gaim plugin that allows you to configure the language of the spelling
 * control on the conversation screen on a per-contact basis.
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

#define GAIM_PLUGINS
#define PLUGIN "core-kleptog-buddylang"
#define SETTING_NAME "language"
#define CONTROL_NAME PLUGIN "-" SETTING_NAME

#include <glib.h>
#include <string.h>

#include "gaim-compat.h"

#include "notify.h"
#include "plugin.h"
#include "version.h"

#include "debug.h"              /* Debug output functions */
#include "util.h"               /* Menu functions */
#include "request.h"            /* Requests stuff */
#include "conversation.h"       /* Conversation stuff */

#include "gtkconv.h"

#include "aspell.h"
#include "gtkspell/gtkspell.h"

static GaimPlugin *plugin_self;

#define LANGUAGE_FLAG ((void*)1)
#define DISABLED_FLAG ((void*)2)

/* Resolve specifies what the return value should mean:
 *
 * If TRUE, it's for display, we want to know the *effect* thus hiding the
 * "none" value and going to going level to find the default
 *
 * If false, we only want what the user enter, thus the string "none" if
 * that's what it is
 */
static const char *
buddy_get_language(GaimBlistNode * node, gboolean resolve)
{
    GaimBlistNode *datanode = NULL;
    const char *language;

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

    language = gaim_blist_node_get_string(datanode, SETTING_NAME);

    if(!resolve)
        return language;

    if(language && strcmp(language, "none") == 0)
        return NULL;

    if(language)
        return language;

    if(datanode->type == GAIM_BLIST_CONTACT_NODE)
    {
        /* There is no gaim_blist_contact_get_group(), though there probably should be */
        datanode = datanode->parent;
        language = gaim_blist_node_get_string(datanode, SETTING_NAME);
    }

    if(language && strcmp(language, "none") == 0)
        return NULL;

    return language;
}

static void
buddylang_createconv_cb(GaimConversation * conv, void *data)
{
    const char *name;
    GaimBuddy *buddy;
    const char *language;
#ifdef PIDGIN_CONVERSATION
    PidginConversation *gtkconv;
#else
    GaimGtkConversation *gtkconv;
#endif
    GtkSpell *gtkspell;
    char *str;
    GError *error = NULL;

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

    language = buddy_get_language((GaimBlistNode *) buddy, TRUE);

    if(!language)
        return;

#ifdef PIDGIN_CONVERSATION
    gtkconv = PIDGIN_CONVERSATION(conv);
#else
    gtkconv = GAIM_GTK_CONVERSATION(conv);
#endif
    gtkspell = gtkspell_get_from_text_view(GTK_TEXT_VIEW(gtkconv->entry));

    if(!gtkspell)
        return;

    if(strcmp(language, "none"))
        gtkspell_detach(gtkspell);
    else if(!gtkspell_set_language(gtkspell, language, &error) && error)
    {
        gaim_debug_warning(PLUGIN, "Failed to configure GtkSpell for language %s: %s\n",
                           language, error->message);
        g_error_free(error);
    }
    str = g_strdup_printf("Spellcheck language: %s", language);
    gaim_conversation_write(conv, PLUGIN, str, GAIM_MESSAGE_SYSTEM, time(NULL));
    g_free(str);
}

static void
buddylang_submitfields_cb(GaimRequestFields * fields, GaimBlistNode * data)
{
    GaimBlistNode *node;
    GaimRequestField *list;
    const GList *sellist;
    void *seldata = NULL;

    gaim_debug(GAIM_DEBUG_INFO, PLUGIN, "buddylang_submitfields_cb(%p,%p)\n", fields, data);

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
    if(seldata == LANGUAGE_FLAG)
        gaim_blist_node_set_string(node, SETTING_NAME, sellist->data);
    else if(seldata == DISABLED_FLAG)
        gaim_blist_node_set_string(node, SETTING_NAME, "none");
    else
        gaim_blist_node_remove_setting(node, SETTING_NAME);
}

/* Node is either a contact or a buddy */
static void
buddylang_createfields_cb(GaimRequestFields * fields, GaimBlistNode * data)
{
    gaim_debug(GAIM_DEBUG_INFO, PLUGIN, "buddylang_createfields_cb(%p,%p)\n", fields, data);
    GaimRequestField *field;
    GaimRequestFieldGroup *group;
    const char *language;
    gboolean is_default;

    struct AspellConfig *aspellconfig;
    struct AspellDictInfoList *dictinfolist;
    struct AspellDictInfoEnumeration *dictinfoenumeration;

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
                                    is_default ? "Default spellcheck language for group" :
                                    "Spellcheck language");
    gaim_request_field_list_set_multi_select(field, FALSE);
    gaim_request_field_list_add(field, "<Default>", "");
    gaim_request_field_list_add(field, "<Disabled>", DISABLED_FLAG);

    /* I'd love to be able to access the gtkspell one, but it's hidden, so we create our own */
    aspellconfig = new_aspell_config();
    if(!aspellconfig)
    {
        gaim_debug(GAIM_DEBUG_INFO, PLUGIN, "new_aspell_config() returned NULL\n");
        return;
    }
    dictinfolist = get_aspell_dict_info_list(aspellconfig);
    if(!dictinfolist)
    {
        gaim_debug(GAIM_DEBUG_INFO, PLUGIN, "get_aspell_dict_info_list() returned NULL\n");
        return;
    }
    gaim_debug(GAIM_DEBUG_INFO, PLUGIN, "dictinfo size = %d\n",
               aspell_dict_info_list_size(dictinfolist));
    dictinfoenumeration = aspell_dict_info_list_elements(dictinfolist);
    if(!dictinfoenumeration)
    {
        gaim_debug(GAIM_DEBUG_INFO, PLUGIN, "aspell_dict_info_list_elements() returned NULL\n");
        return;
    }

    /* We have a list of dictionaries, but we can only specify a language
     * (like en_GB). Since the language can appear multiple times on the
     * list, we check before adding to avoid duplicates */
    while (!aspell_dict_info_enumeration_at_end(dictinfoenumeration))
    {
        const struct AspellDictInfo *dictinfo =
            aspell_dict_info_enumeration_next(dictinfoenumeration);

        if(gaim_request_field_list_get_data(field, dictinfo->code) != NULL)
            continue;
        gaim_request_field_list_add(field, dictinfo->code, LANGUAGE_FLAG);
    }
    delete_aspell_dict_info_enumeration(dictinfoenumeration);
    delete_aspell_config(aspellconfig);

    language = buddy_get_language(data, FALSE);
    if(language)
    {
        if(strcmp(language, "none") == 0)
            gaim_request_field_list_add_selected(field, "<Disabled>");
        else
            gaim_request_field_list_add_selected(field, language);
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
                        GAIM_CALLBACK(buddylang_createfields_cb), NULL);
    gaim_signal_connect(gaim_blist_get_handle(), "core-kleptog-buddyedit-submit-fields", plugin,
                        GAIM_CALLBACK(buddylang_submitfields_cb), NULL);
    gaim_signal_connect(gaim_conversations_get_handle(), "conversation-created", plugin,
                        GAIM_CALLBACK(buddylang_createconv_cb), NULL);

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
    "Buddy Language Module",
    G_STRINGIFY(PLUGIN_VERSION),

    "Configure spell-check language per buddy",
    "This plugin allows you to configure the language of the spelling control on the conversation screen on a per-contact basis.",
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

GAIM_INIT_PLUGIN(buddylang, init_plugin, info);

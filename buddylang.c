/*************************************************************************
 * Buddy Language Module
 *
 * A Purple plugin that allows you to configure the language of the spelling
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

#define PURPLE_PLUGINS
#define PLUGIN "core-kleptog-buddylang"
#define SETTING_NAME "language"
#define CONTROL_NAME PLUGIN "-" SETTING_NAME

#include <glib.h>
#include <string.h>

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

static PurplePlugin *plugin_self;

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
buddy_get_language(PurpleBlistNode * node, gboolean resolve)
{
    PurpleBlistNode *datanode = NULL;
    const char *language;

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

    language = purple_blist_node_get_string(datanode, SETTING_NAME);

    if(!resolve)
        return language;

    if(language && strcmp(language, "none") == 0)
        return NULL;

    if(language)
        return language;

    if(datanode->type == PURPLE_BLIST_CONTACT_NODE)
    {
        /* There is no purple_blist_contact_get_group(), though there probably should be */
        datanode = datanode->parent;
        language = purple_blist_node_get_string(datanode, SETTING_NAME);
    }

    if(language && strcmp(language, "none") == 0)
        return NULL;

    return language;
}

static void
buddylang_createconv_cb(PurpleConversation * conv, void *data)
{
    const char *name;
    PurpleBuddy *buddy;
    const char *language;
#ifdef PIDGIN_CONVERSATION
    PidginConversation *gtkconv;
#else
    PurpleGtkConversation *gtkconv;
#endif
    GtkSpell *gtkspell;
    char *str;
    GError *error = NULL;

#if PURPLE_MAJOR_VERSION < 2
    if(purple_conversation_get_type(conv) != PURPLE_CONV_IM)
        return;
#else
    if(purple_conversation_get_type(conv) != PURPLE_CONV_TYPE_IM)
        return;
#endif

    name = purple_conversation_get_name(conv);
    buddy = purple_find_buddy(purple_conversation_get_account(conv), name);
    if(!buddy)
        return;

    language = buddy_get_language((PurpleBlistNode *) buddy, TRUE);

    if(!language)
        return;

#ifdef PIDGIN_CONVERSATION
    gtkconv = PIDGIN_CONVERSATION(conv);
#else
    gtkconv = PURPLE_GTK_CONVERSATION(conv);
#endif
    gtkspell = gtkspell_get_from_text_view(GTK_TEXT_VIEW(gtkconv->entry));

    if(!gtkspell)
        return;

    if(strcmp(language, "none"))
        gtkspell_detach(gtkspell);
    else if(!gtkspell_set_language(gtkspell, language, &error) && error)
    {
        purple_debug_warning(PLUGIN, "Failed to configure GtkSpell for language %s: %s\n",
                           language, error->message);
        g_error_free(error);
    }
    str = g_strdup_printf("Spellcheck language: %s", language);
    purple_conversation_write(conv, PLUGIN, str, PURPLE_MESSAGE_SYSTEM, time(NULL));
    g_free(str);
}

static void
buddylang_submitfields_cb(PurpleRequestFields * fields, PurpleBlistNode * data)
{
    PurpleBlistNode *node;
    PurpleRequestField *list;
    const GList *sellist;
    void *seldata = NULL;

    purple_debug(PURPLE_DEBUG_INFO, PLUGIN, "buddylang_submitfields_cb(%p,%p)\n", fields, data);

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
    sellist = purple_request_field_list_get_selected(list);
    if(sellist)
        seldata = purple_request_field_list_get_data(list, sellist->data);

    /* Otherwise, it's fixed value and this means deletion */
    if(seldata == LANGUAGE_FLAG)
        purple_blist_node_set_string(node, SETTING_NAME, sellist->data);
    else if(seldata == DISABLED_FLAG)
        purple_blist_node_set_string(node, SETTING_NAME, "none");
    else
        purple_blist_node_remove_setting(node, SETTING_NAME);
}

/* Node is either a contact or a buddy */
static void
buddylang_createfields_cb(PurpleRequestFields * fields, PurpleBlistNode * data)
{
    purple_debug(PURPLE_DEBUG_INFO, PLUGIN, "buddylang_createfields_cb(%p,%p)\n", fields, data);
    PurpleRequestField *field;
    PurpleRequestFieldGroup *group;
    const char *language;
    gboolean is_default;

    struct AspellConfig *aspellconfig;
    struct AspellDictInfoList *dictinfolist;
    struct AspellDictInfoEnumeration *dictinfoenumeration;

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

    field =
        purple_request_field_list_new(CONTROL_NAME,
                                    is_default ? "Default spellcheck language for group" :
                                    "Spellcheck language");
    purple_request_field_list_set_multi_select(field, FALSE);
    purple_request_field_list_add(field, "<Default>", "");
    purple_request_field_list_add(field, "<Disabled>", DISABLED_FLAG);

    /* I'd love to be able to access the gtkspell one, but it's hidden, so we create our own */
    aspellconfig = new_aspell_config();
    if(!aspellconfig)
    {
        purple_debug(PURPLE_DEBUG_INFO, PLUGIN, "new_aspell_config() returned NULL\n");
        return;
    }
    dictinfolist = get_aspell_dict_info_list(aspellconfig);
    if(!dictinfolist)
    {
        purple_debug(PURPLE_DEBUG_INFO, PLUGIN, "get_aspell_dict_info_list() returned NULL\n");
        return;
    }
    purple_debug(PURPLE_DEBUG_INFO, PLUGIN, "dictinfo size = %d\n",
               aspell_dict_info_list_size(dictinfolist));
    dictinfoenumeration = aspell_dict_info_list_elements(dictinfolist);
    if(!dictinfoenumeration)
    {
        purple_debug(PURPLE_DEBUG_INFO, PLUGIN, "aspell_dict_info_list_elements() returned NULL\n");
        return;
    }

    /* We have a list of dictionaries, but we can only specify a language
     * (like en_GB). Since the language can appear multiple times on the
     * list, we check before adding to avoid duplicates */
    while (!aspell_dict_info_enumeration_at_end(dictinfoenumeration))
    {
        const struct AspellDictInfo *dictinfo =
            aspell_dict_info_enumeration_next(dictinfoenumeration);

        if(purple_request_field_list_get_data(field, dictinfo->code) != NULL)
            continue;
        purple_request_field_list_add(field, dictinfo->code, LANGUAGE_FLAG);
    }
    delete_aspell_dict_info_enumeration(dictinfoenumeration);
    delete_aspell_config(aspellconfig);

    language = buddy_get_language(data, FALSE);
    if(language)
    {
        if(strcmp(language, "none") == 0)
            purple_request_field_list_add_selected(field, "<Disabled>");
        else
            purple_request_field_list_add_selected(field, language);
    }
    else
        purple_request_field_list_add_selected(field, "<Default>");

    purple_request_field_group_add_field(group, field);
}

static gboolean
plugin_load(PurplePlugin * plugin)
{

    plugin_self = plugin;

    purple_signal_connect(purple_blist_get_handle(), "core-kleptog-buddyedit-create-fields", plugin,
                        PURPLE_CALLBACK(buddylang_createfields_cb), NULL);
    purple_signal_connect(purple_blist_get_handle(), "core-kleptog-buddyedit-submit-fields", plugin,
                        PURPLE_CALLBACK(buddylang_submitfields_cb), NULL);
    purple_signal_connect(purple_conversations_get_handle(), "conversation-created", plugin,
                        PURPLE_CALLBACK(buddylang_createconv_cb), NULL);

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
    "Buddy Language Module",
    G_STRINGIFY(PLUGIN_VERSION),

    "Configure spell-check language per buddy",
    "This plugin allows you to configure the language of the spelling control on the conversation screen on a per-contact basis.",
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

PURPLE_INIT_PLUGIN(buddylang, init_plugin, info);

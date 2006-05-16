/*************************************************************************
 * Buddy Language Module
 * by Martijn van Oosterhout <kleptog@svana.org> (C) April 2006
 * Licenced under the GNU General Public Licence version 2.
 *
 * A GAIM plugin that allows you to configure the language of the spelling
 * control on the conversation screen on a per-contact basis.
 *************************************************************************/

#define GAIM_PLUGINS

#include <glib.h>
#include <ctype.h>

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

static const char *
buddy_get_language(GaimBlistNode * node)
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

    return gaim_blist_node_get_string((GaimBlistNode *) contact, "language");
}

static void
buddylang_createconv_cb(GaimConversation * conv, void *data)
{
    const char *name;
    GaimBuddy *buddy;
    const char *language;
    GaimGtkConversation *gtkconv;
    GtkSpell *gtkspell;
    char *str;
    GError *error = NULL;

    if(gaim_conversation_get_type(conv) != GAIM_CONV_IM)
        return;

    name = gaim_conversation_get_name(conv);
    buddy = gaim_find_buddy(gaim_conversation_get_account(conv), name);
    if(!buddy)
        return;

    language = buddy_get_language((GaimBlistNode *) buddy);

    if(!language)
        return;

    gtkconv = GAIM_GTK_CONVERSATION(conv);
    gtkspell = gtkspell_get_from_text_view(GTK_TEXT_VIEW(gtkconv->entry));

    if(!gtkspell)
        return;

    if(!gtkspell_set_language(gtkspell, language, &error) && error)
    {
        gaim_debug_warning("buddylang", "Failed to configure GtkSpell for language %s: %s\n",
                           language, error->message);
        g_error_free(error);
    }
    str = g_strdup_printf("Spellcheck language: %s", language);
    gaim_conversation_write(conv, "gaim-buddylang", str, GAIM_MESSAGE_SYSTEM, time(NULL));
    g_free(str);
}

static void
buddylang_submitfields_cb(GaimRequestFields * fields, GaimBlistNode * data)
{
    GaimContact *contact;
    GaimRequestField *list;
    const GList *sellist;
    gboolean is_language;

    gaim_debug(GAIM_DEBUG_INFO, "gaim-buddylang", "buddylang_submitfields_cb(%p,%p)\n", fields,
               data);
    if(GAIM_BLIST_NODE_IS_BUDDY(data))
        contact = gaim_buddy_get_contact((GaimBuddy *) data);
    else                        /* Contact */
        contact = (GaimContact *) data;

    list = gaim_request_fields_get_field(fields, "language");
    sellist = gaim_request_field_list_get_selected(list);
    is_language = (sellist
                   && gaim_request_field_list_get_data(list, sellist->data) == LANGUAGE_FLAG);

    /* Otherwise, it's fixed value and this means deletion */
    if(is_language)
        gaim_blist_node_set_string((GaimBlistNode *) contact, "language", sellist->data);
    else
        gaim_blist_node_remove_setting((GaimBlistNode *) contact, "language");
}

/* Node is either a contact or a buddy */
static void
buddylang_createfields_cb(GaimRequestFields * fields, GaimBlistNode * data)
{
    gaim_debug(GAIM_DEBUG_INFO, "gaim-buddylang", "buddylang_createfields_cb(%p,%p)\n", fields,
               data);
    GaimRequestField *field;
    GaimRequestFieldGroup *group;
    const char *language;

    struct AspellConfig *aspellconfig;
    struct AspellDictInfoList *dictinfolist;
    struct AspellDictInfoEnumeration *dictinfoenumeration;

    group = gaim_request_field_group_new(NULL);
    gaim_request_fields_add_group(fields, group);

    field = gaim_request_field_list_new("language", "Spellcheck language");
    gaim_request_field_list_set_multi_select(field, FALSE);
    gaim_request_field_list_add(field, "<Disabled>", "");

    /* I'd love to be able to access the gtkspell one, but it's hidden, so we create our own */
    aspellconfig = new_aspell_config();
    if(!aspellconfig)
    {
        gaim_debug(GAIM_DEBUG_INFO, "gaim-buddylang", "new_aspell_config() returned NULL\n");
        return;
    }
    dictinfolist = get_aspell_dict_info_list(aspellconfig);
    if(!dictinfolist)
    {
        gaim_debug(GAIM_DEBUG_INFO, "gaim-buddylang",
                   "get_aspell_dict_info_list() returned NULL\n");
        return;
    }
    gaim_debug(GAIM_DEBUG_INFO, "gaim-buddylang", "dictinfo size = %d\n",
               aspell_dict_info_list_size(dictinfolist));
    dictinfoenumeration = aspell_dict_info_list_elements(dictinfolist);
    if(!dictinfoenumeration)
    {
        gaim_debug(GAIM_DEBUG_INFO, "gaim-buddylang",
                   "aspell_dict_info_list_elements() returned NULL\n");
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

    language = buddy_get_language(data);
    if(language)
        gaim_request_field_list_add_selected(field, language);

    gaim_request_field_group_add_field(group, field);
}

static gboolean
plugin_load(GaimPlugin * plugin)
{

    plugin_self = plugin;

    gaim_signal_connect(gaim_blist_get_handle(), "buddyedit-create-fields", plugin,
                        GAIM_CALLBACK(buddylang_createfields_cb), NULL);
    gaim_signal_connect(gaim_blist_get_handle(), "buddyedit-submit-fields", plugin,
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

    "core-kleptog-buddylang",
    "Buddy Language Module",
    "0.1",

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

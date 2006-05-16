/*************************************************************************
 * Buddy Edit Module
 * by Martijn van Oosterhout <kleptog@svana.org> (C) April 2006
 * Licenced under the GNU General Public Licence version 2.
 *
 * A GAIM plugin that adds an edit to to buddies allowing you to change
 * various details you can't normally change. It also provides a mechanism
 * for subsequent plugins to add themselves to that dialog.
 *************************************************************************/

#define GAIM_PLUGINS
#define PLUGIN "core-kleptog-buddyedit"

#include <glib.h>
#include <string.h>
#include <ctype.h>

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
    if(GAIM_BLIST_NODE_IS_BUDDY(data))
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
            gaim_blist_add_buddy(newbuddy, NULL, NULL, data);   /* Copy it to correct location */

            /* Now this is ugly, but we want to copy the settings and avoid issues with memory management */
            tmp = ((GaimBlistNode *) buddy)->settings;
            ((GaimBlistNode *) buddy)->settings = ((GaimBlistNode *) newbuddy)->settings;
            ((GaimBlistNode *) newbuddy)->settings = tmp;

            buddy_destroy = TRUE;
            data = (GaimBlistNode *) newbuddy;
        }
        else
            gaim_blist_alias_buddy(buddy, alias);
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

    fields = gaim_request_fields_new();

    /* First group: Account details */
    if(GAIM_BLIST_NODE_IS_BUDDY(node))
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
    }

    gaim_signal_emit(gaim_blist_get_handle(), PLUGIN "-create-fields", fields, node);

    gaim_request_fields(plugin_self,
                        "Edit Contact", NULL, NULL,
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

    if(!GAIM_BLIST_NODE_IS_BUDDY(node) && !GAIM_BLIST_NODE_IS_CONTACT(node))
        return;

#if GAIM_MAJOR_VERSION < 2
    action = gaim_blist_node_action_new("Edit...", buddy_edit_cb, NULL);
#else
    action = gaim_menu_action_new("Edit...", buddy_edit_cb, NULL, NULL);
#endif
    *menu = g_list_append(*menu, action);
}

static gboolean
plugin_load(GaimPlugin * plugin)
{

    plugin_self = plugin;

    void *blist_handle = gaim_blist_get_handle();

    gaim_signal_register(blist_handle, PLUGIN "-create-fields",       /* Called when about to create dialog */
                         gaim_marshal_VOID__POINTER_POINTER, NULL, 2,   /* (FieldList*,BlistNode*) */
                         gaim_value_new(GAIM_TYPE_SUBTYPE, GAIM_TYPE_POINTER),  /* FieldList */
                         gaim_value_new(GAIM_TYPE_SUBTYPE, GAIM_SUBTYPE_BLIST));

    gaim_signal_register(blist_handle, PLUGIN "-submit-fields",       /* Called when dialog submitted */
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

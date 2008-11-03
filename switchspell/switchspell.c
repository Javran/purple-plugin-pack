/*
 * Switchspell - Switch spelling language during run time.
 * Copyright (C) 2007-2008
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02111-1301, USA.
 */

/* If you can't figure out what this line is for, DON'T TOUCH IT. */
#include "../common/pp_internal.h"

#define PLUGIN_ID           "gtk-plugin_pack-switchspell"
#define PLUGIN_STATIC_NAME  "switchspell"
#define PLUGIN_AUTHOR       "Alfredo Raul Pena (arpena)\n" \
                            "Sadrul H Chowdhury <sadrul@users.sourceforge.net>"

/* Purple headers */
#include <conversation.h>
#include <debug.h>
#include <plugin.h>

/* Pidgin headers */
#include <gtkconv.h>
#include <gtkplugin.h>

#ifdef USE_ENCHANT
#include <enchant.h>
#else
#include <aspell.h>
#endif
#include <gtkspell/gtkspell.h>
#include <string.h>

#define PROP_LANG     "switchspell::language"

static PurpleBlistNode *
blist_node_for_conv(PurpleConversation *conv)
{
	PurpleBlistNode *node = NULL;

	switch (purple_conversation_get_type(conv)) {
		case PURPLE_CONV_TYPE_IM:
		{
			PurpleBuddy *buddy = purple_find_buddy(conv->account, conv->name);
			if (buddy)
				node = (PurpleBlistNode *)purple_buddy_get_contact(buddy);
			break;
		}
		case PURPLE_CONV_TYPE_CHAT:
			node = (PurpleBlistNode *)purple_blist_find_chat(conv->account, conv->name);
			break;
		default:
			break;
	}
	return node;
}

static void
menu_conv_use_dict_cb(GObject *m, gpointer data)
{
	PidginWindow *win = g_object_get_data(m, "user_data");
	gchar *lang = g_object_get_data(m, "lang");
	PurpleConversation *conv;
	PidginConversation *gtkconv;
	GtkSpell *spell;
	GError *error = NULL;
	PurpleBlistNode *node;

	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(m)))
		return;

	conv = pidgin_conv_window_get_active_conversation(win);

	gtkconv = PIDGIN_CONVERSATION(conv);
	spell = gtkspell_get_from_text_view(GTK_TEXT_VIEW(gtkconv->entry));
	if (spell != NULL) {
		if (!gtkspell_set_language(spell, lang, &error)) {
			purple_debug_error("switchspell", "failed to set language %s: %s\n", lang, error->message);
			g_error_free(error);
		}
	}
	g_object_set_data(G_OBJECT(gtkconv->entry), PROP_LANG, lang);

	node = blist_node_for_conv(gtkconv->active_conv);
	if (node)
		purple_blist_node_set_string(node, "switchspell", lang);
}

#ifdef USE_ENCHANT
struct nufan
{
	GSList *group;
	GtkWidget *menu;
	PidginWindow *win;
};

static void
enchant_dict_desc_cb(const char * const lang_tag, const char * const provider_name,
                     const char * const provider_desc, const char * const provider_file,
                     void *user_data)
{
	GtkWidget *menu = ((struct nufan *)user_data)->menu;
	PidginWindow *win = ((struct nufan *)user_data)->win;

	GtkWidget *menuitem = gtk_radio_menu_item_new_with_label(group, lang_tag);
	((struct nufan *)user_data)->group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem));
	g_object_set_data(G_OBJECT(menuitem), "user_data", win);
	g_object_set_data_full(G_OBJECT(menuitem), "lang", g_strdup(lang_tag), g_free);
	g_signal_connect(G_OBJECT(menuitem), "activate",
					G_CALLBACK(menu_conv_use_dict_cb), NULL);
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
}
#endif

static void
regenerate_switchspell_menu(PidginConversation *gtkconv)
{
	PidginWindow *win;
	GtkWidget *menu;
	GtkWidget *mitem;
#ifdef USE_ENCHANT
	struct nufan user_data;
	EnchantBroker *eb;
#else
	GSList *group = NULL;
	AspellConfig *config;
	AspellDictInfoList *dlist;
	AspellDictInfoEnumeration *dels;
	const AspellDictInfo *entry;
#endif

	if (gtkconv == NULL)
		return;

	win = pidgin_conv_get_window(gtkconv);
	if (win == NULL)
		return;

	mitem = g_object_get_data(G_OBJECT(win->window), PROP_LANG);
	if (mitem == NULL) {
		mitem = gtk_menu_item_new_with_mnemonic(_("Spe_ll Check"));
		gtk_menu_shell_insert(GTK_MENU_SHELL(win->menu.menubar), mitem, 3);
		g_object_set_data(G_OBJECT(win->window), PROP_LANG, mitem);
		gtk_widget_show(mitem);
	}
	else
		return;

	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(mitem), menu);

#ifdef USE_ENCHANT
	user_data.group = NULL;
	user_data.menu = menu;
	user_data.win = win;
	eb = enchant_broker_init();
	enchant_broker_list_dicts(eb, enchant_dict_desc_cb, &user_data);
	enchant_broker_free(eb);
#else
	config = new_aspell_config();
	dlist = get_aspell_dict_info_list(config);
	delete_aspell_config(config);

	dels = aspell_dict_info_list_elements(dlist);
	aspell_dict_info_list_empty(dlist);
	while ((entry = aspell_dict_info_enumeration_next(dels)) != 0) {
		GtkWidget *menuitem = gtk_radio_menu_item_new_with_label(group, entry->name);
		group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem));
		g_object_set_data(G_OBJECT(menuitem), "user_data", win);
		g_object_set_data_full(G_OBJECT(menuitem), "lang", g_strdup((gchar *)entry->name), g_free);
		g_signal_connect(G_OBJECT(menuitem), "activate",
					G_CALLBACK(menu_conv_use_dict_cb), NULL);
		gtk_widget_show(menuitem);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	}
	delete_aspell_dict_info_enumeration(dels);
#endif
	gtk_widget_show_all(menu);
}

static void
update_switchspell_selection(PidginConversation *gtkconv)
{
	PidginWindow *win;
	GtkWidget *menu;
	GList *item, *items;
	char *curlang;

	if (gtkconv == NULL)
		return;

	win = pidgin_conv_get_window(gtkconv);
	if (win == NULL)
		return;

	/* Leave the menu alone if the tab is not the active conversation */
	if (pidgin_conv_window_get_active_gtkconv(win) != gtkconv)
		return;

	menu = g_object_get_data(G_OBJECT(win->window), PROP_LANG);
	if (menu == NULL)
		return;
	menu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(menu));

	curlang = g_strdup(g_object_get_data(G_OBJECT(gtkconv->entry), PROP_LANG));

	items = gtk_container_get_children(GTK_CONTAINER(menu));
	g_list_foreach(items,
				(GFunc)gtk_check_menu_item_set_active, GINT_TO_POINTER(FALSE));
	g_list_free(items);

	items = gtk_container_get_children(GTK_CONTAINER(menu));
	for (item = items; item; item = item->next) {
		const char *lang = g_object_get_data(G_OBJECT(item->data), "lang");
		if (lang && curlang && strcmp(lang, curlang) == 0) {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item->data), TRUE);
			break;
		}
	}
	g_list_free(items);
	g_free(curlang);
}

static gboolean make_sure_gtkconv(PurpleConversation *conv);

static void
conversation_switched_cb(PurpleConversation *conv)
{
	PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);
	if (!g_object_get_data(G_OBJECT(gtkconv->entry), PROP_LANG))
		make_sure_gtkconv(conv);
	regenerate_switchspell_menu(gtkconv);
	update_switchspell_selection(gtkconv);
}

static gboolean
make_sure_gtkconv(PurpleConversation *conv)
{
	PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);
	const char *lang = NULL;
	PurpleBlistNode *node;
	if (gtkconv == NULL)
		return TRUE;

	node = blist_node_for_conv(conv);
	if (node)
		lang = purple_blist_node_get_string(node, "switchspell");
	g_object_set_data(G_OBJECT(gtkconv->entry), PROP_LANG, lang ? (char *)lang : getenv("LANG"));
	update_switchspell_selection(gtkconv);
	return FALSE;
}

static void
conversation_created_cb(PurpleConversation *conv)
{
	/* Read all about it (and more!) in xmmsremote */
	g_timeout_add(500, (GSourceFunc)make_sure_gtkconv, conv);
}

static void attach_switchspell_menu(gpointer data, gpointer dontcare)
{
	PidginWindow *win = data;
	PidginConversation *gtkconv;

	gtkconv = pidgin_conv_window_get_active_gtkconv(win);
	regenerate_switchspell_menu(gtkconv);
	update_switchspell_selection(gtkconv);
}

static gboolean plugin_load(PurplePlugin *plugin)
{
	void *conv_handle = purple_conversations_get_handle();

	purple_signal_connect(conv_handle, "conversation-created", plugin,
						PURPLE_CALLBACK(conversation_created_cb), NULL);
	purple_signal_connect(pidgin_conversations_get_handle(), "conversation-switched",
						plugin, PURPLE_CALLBACK(conversation_switched_cb), NULL);

	g_list_foreach(pidgin_conv_windows_get_list(), attach_switchspell_menu, NULL);

	return TRUE;
}

static void remove_switchspell_menu(gpointer data, gpointer dontcare)
{
	PidginWindow *win = data;
	GtkWidget *menu;

	menu = g_object_get_data(G_OBJECT(win->window), PROP_LANG);
	if (menu) {
		gtk_widget_destroy(menu);
		g_object_set_data(G_OBJECT(win->window), PROP_LANG, NULL);
	}
}

static gboolean plugin_unload(PurplePlugin *plugin)
{
	purple_prefs_disconnect_by_handle(plugin);

	g_list_foreach(pidgin_conv_windows_get_list(), remove_switchspell_menu, NULL);

	return TRUE;
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,        /* Magic               */
	PURPLE_MAJOR_VERSION,       /* Purple Major Version  */
	PURPLE_MINOR_VERSION,       /* Purple Minor Version  */
	PURPLE_PLUGIN_STANDARD,     /* plugin type         */
	PIDGIN_PLUGIN_TYPE,         /* ui requirement      */
	0,                          /* flags               */
	NULL,                       /* dependencies        */
	PURPLE_PRIORITY_DEFAULT,    /* priority            */

	PLUGIN_ID,                  /* plugin id           */
	NULL,                       /* name                */
	PP_VERSION,                 /* version             */
	NULL,                       /* summary             */
	NULL,                       /* description         */
	PLUGIN_AUTHOR,              /* author              */
	PP_WEBSITE,                 /* website             */

	plugin_load,                /* load                */
	plugin_unload,              /* unload              */
	NULL,                       /* destroy             */

	NULL,                       /* ui_info             */
	NULL,                       /* extra_info          */
	NULL,                       /* prefs_info          */
	NULL,                       /* actions             */
	NULL,                       /* reserved 1          */
	NULL,                       /* reserved 2          */
	NULL,                       /* reserved 3          */
	NULL                        /* reserved 4          */
};

static void init_plugin(PurplePlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Switch Spell");
	info.summary = _("Switch Spell Checker Language");
	info.description = _("Switch Spell Checker Language");
}

PURPLE_INIT_PLUGIN(switchspell, init_plugin, info)

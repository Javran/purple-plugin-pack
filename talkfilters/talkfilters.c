/*
 * A neat little Gaim plugin to integrate with GNU Talk Filters.
 * http://www.hyperrealm.com/talkfilters/talkfilters.html
 *
 * Mark Lindner <markl@gnu.org> 1/6/04
 * Updates for the gaim plugin pack (C) 2005 by
 *    Peter Lawler <bleeter from users.sf.net>
 */

/* TODO:
-- save filter preference
	-- done (sadrul)
-- slash commands (allowing it to be a one liner)
-- convert all gtk preference calls to gaim internal calls
	-- done (sadrul)
-- allow saving different filters for different buddies (or accounts)
-- add an entry in the popup for the imhtml-entry in the conversation window
   to allow changing the filter for that conversation. (so that I can quickly
   turn it on when nosnilmot or Paco-Paco is around)
   	-- done (added a menuitem in the conversation window instead)
*/

#if HAVE_CONFIG_H
# include "../gpp_config.h"
#endif /* HAVE_CONFIG_H */

#define GAIM_PLUGINS

#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <strings.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#include <debug.h>
#include <signals.h>
#include <util.h>
#include <version.h>

#include <gtkplugin.h>
#include <gtkutils.h>

#include "../common/i18n.h"

#include <talkfilters.h>

#define	PREF_PREFIX     "/plugins/gtk/bleeter/talkfilters"
#define	PREF_ENABLED    PREF_PREFIX "/enabled"
#define	PREF_FILTER     PREF_PREFIX "/filter"

#define PROP_FILTER     "talkfilter::filter"

static const gtf_filter_t *current_filter = NULL;
static const gtf_filter_t *filter_list = NULL;
static int filter_count = 0;

static void regenerate_talkfilter_menu(GaimGtkConversation *gtkconv);

static void
translate_message(char **message, const gtf_filter_t *filter) {
	if (message == NULL || *message == NULL) {
		gaim_debug_info("talkfilters","Null message\n");
		return;
	}

	if(filter != NULL) {
		gchar *tmp;

		size_t len = strlen(*message);
		if(len < 40)
			len += 40;
		else
			len *= 2;

		/* XXX: Is it always true, or are we just hoping it is? */
		tmp = (gchar *)g_malloc(len);

		filter->filter(*message, tmp, len);
		g_free(*message);
		*message = tmp;
	} else {
		gaim_debug_info("talkfilters","No filter set\n");
	}
}

static void translate_message_im(GaimAccount *account, char *who,
								  char **message, gpointer dontcare) {
	GaimConversation *conv;
	GaimGtkConversation *gtkconv;

	conv = gaim_find_conversation_with_account(GAIM_CONV_TYPE_IM, who, account);
	if (!conv)
		return;
	gtkconv = GAIM_GTK_CONVERSATION(conv);
	if (!gtkconv)
		return;

	translate_message(message, g_object_get_data(G_OBJECT(gtkconv->imhtml), PROP_FILTER));
}

static void translate_message_chat(GaimAccount *account, char **message,
									int id, gpointer dontcare) {
	GaimConversation *conv;
	GaimGtkConversation *gtkconv;
   
	conv = gaim_find_chat(account->gc, id);
	if (!conv)
		return;

	gtkconv = GAIM_GTK_CONVERSATION(conv);
	if (!gtkconv)
		return;

	translate_message(message, g_object_get_data(G_OBJECT(gtkconv->imhtml), PROP_FILTER));
}

static void
update_selected_filter() {
	const gtf_filter_t *filter;
	gint ct;
	const char *val = gaim_prefs_get_string(PREF_FILTER);

	current_filter = NULL;
	ct = filter_count;
	for(filter = filter_list; ct; filter++, ct--) {
		/* XXX: Is this overkill? Is strcmp enough? */
		if (g_utf8_collate(val, filter->name) == 0) {
			current_filter = filter;
			gaim_debug_info("talkfilters", "found default filter \"%s\"\n", filter->name);
			break;
		}
	}
}

static void
filter_changed_cb(const char *name, GaimPrefType type, gconstpointer val, gpointer dontcare) {
	update_selected_filter();
}

static gboolean writing_im_msg(GaimAccount *account, const char *who, char **message,
				GaimConversation *conv, GaimMessageFlags flags, gpointer dontcare) {
	if (flags & GAIM_MESSAGE_SEND)
	{
		GaimGtkConversation *gtkconv = GAIM_GTK_CONVERSATION(conv);
		if (!gtkconv)
			return FALSE;

		translate_message(message, g_object_get_data(G_OBJECT(gtkconv->imhtml), PROP_FILTER));
	}
	return FALSE;
}

static void
menu_filter_changed_cb(GtkWidget *w, GaimGtkWindow *win)
{
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
	{
		GaimGtkConversation *gtkconv = gaim_gtk_conv_window_get_active_gtkconv(win);
		g_object_set_data(G_OBJECT(gtkconv->imhtml), PROP_FILTER,
				g_object_get_data(G_OBJECT(w), PROP_FILTER));
	}
}

static void
regenerate_talkfilter_menu(GaimGtkConversation *gtkconv)
{
	GaimGtkWindow *win;
	GtkWidget *menu;
	int count;
	const gtf_filter_t *filter;
	const gtf_filter_t *curfilter;
	GtkWidget *mitem, *item;
	GSList *list = NULL;

	if (gtkconv == NULL)
		return;

	win = gaim_gtkconv_get_window(gtkconv);
	if (win == NULL)
		return;

	mitem = g_object_get_data(G_OBJECT(win->window), PROP_FILTER);
	if (mitem == NULL)
	{
		mitem = gtk_menu_item_new_with_mnemonic(_("_Talkfilters"));	/* XXX: or is it "Talk Filters"? */
		gtk_menu_shell_insert(GTK_MENU_SHELL(win->menu.menubar), mitem, 3);
		g_object_set_data(G_OBJECT(win->window), PROP_FILTER, mitem);
		gtk_widget_show(mitem);
	}
	else
		return;

	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(mitem), menu);

	curfilter = g_object_get_data(G_OBJECT(gtkconv->imhtml), PROP_FILTER);

	item = gtk_radio_menu_item_new_with_label(list, _("(None)"));
	list = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(menu_filter_changed_cb), win);

	for (count = filter_count, filter = filter_list; count; filter++, count--)
	{
		item = gtk_radio_menu_item_new_with_label(list, filter->desc);
		g_object_set_data(G_OBJECT(item), PROP_FILTER, (gpointer)filter);
		list = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));

		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(menu_filter_changed_cb), win);

		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	}

	gtk_widget_show_all(menu);
}

static void
update_talkfilter_selection(GaimGtkConversation *gtkconv)
{
	GaimGtkWindow *win;
	GtkWidget *menu;
	GList *item;
	const gtf_filter_t *filter;

	if (gtkconv == NULL)
		return;

	win = gaim_gtkconv_get_window(gtkconv);
	if (win == NULL)
		return;

	menu = g_object_get_data(G_OBJECT(win->window), PROP_FILTER);
	if (menu == NULL)
		return;
	menu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(menu));

	filter = g_object_get_data(G_OBJECT(gtkconv->imhtml), PROP_FILTER);

	for (item = gtk_container_get_children(GTK_CONTAINER(menu));
				item; item = item->next)
	{
		if (filter == g_object_get_data(G_OBJECT(item->data), PROP_FILTER))
		{
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item->data), TRUE);
			break;
		}		
	}
}

static void
conversation_switched_cb(GaimConversation *conv)
{
	GaimGtkConversation *gtkconv = GAIM_GTK_CONVERSATION(conv);
	regenerate_talkfilter_menu(gtkconv);
	update_talkfilter_selection(gtkconv);
}
		
static void
conversation_created_cb(GaimConversation *conv)
{
	GaimGtkConversation *gtkconv = GAIM_GTK_CONVERSATION(conv);
	g_object_set_data(G_OBJECT(gtkconv->imhtml), PROP_FILTER, (gpointer)current_filter);
	update_talkfilter_selection(gtkconv);
}

static void attach_talkfilter_menu(gpointer data, gpointer dontcare)
{
	GaimGtkWindow *win = data;
	GaimGtkConversation *gtkconv;

	gtkconv = gaim_gtk_conv_window_get_active_gtkconv(win);
	regenerate_talkfilter_menu(gtkconv);
	update_talkfilter_selection(gtkconv);
}

static gboolean plugin_load(GaimPlugin *plugin)
{
	void *conv_handle = gaim_conversations_get_handle();

	filter_list = gtf_filter_list();
	filter_count = gtf_filter_count();
	update_selected_filter();

	gaim_signal_connect(conv_handle, "sending-im-msg",
						plugin, GAIM_CALLBACK(translate_message_im), NULL);
	gaim_signal_connect(conv_handle, "sending-chat-msg",
						plugin, GAIM_CALLBACK(translate_message_chat), NULL);

	/* XXX: This is necessary because the changed message isn't displayed locally.
	 * 		This doesn't always show the exact filtered message that is sent, but
	 * 		I guess it's better than no indication that the message was filtered.
	 * 			-- sadrul
	 */
	gaim_signal_connect(conv_handle, "writing-im-msg", plugin,
						GAIM_CALLBACK(writing_im_msg), NULL);
	
	gaim_prefs_connect_callback(plugin, PREF_FILTER,
							filter_changed_cb, NULL);

	/* Add a `Talkfilters' menu in the conversation window */
	gaim_signal_connect(conv_handle, "conversation-created", plugin,
						GAIM_CALLBACK(conversation_created_cb), NULL);
	gaim_signal_connect(gaim_gtk_conversations_get_handle(), "conversation-switched",
						plugin, GAIM_CALLBACK(conversation_switched_cb), NULL);

	g_list_foreach(gaim_gtk_conv_windows_get_list(), attach_talkfilter_menu, NULL);

	return TRUE;
}

static void remove_talkfilter_menu(gpointer data, gpointer dontcare)
{
	GaimGtkWindow *win = data;
	GtkWidget *menu;

	menu = g_object_get_data(G_OBJECT(win->window), PROP_FILTER);
	if (menu)
	{
		gtk_widget_destroy(menu);
		g_object_set_data(G_OBJECT(win->window), PROP_FILTER, NULL);
		
		/* XXX: Do we need to set PROP_FILTER data to NULL for each gtkconv->imhtml as well?
		 * It doesn't seem to be necessary right now. The GTF library probably gets loaded
		 * at the very beginning when Gaim starts, and not when this plugin is loaded. */
	}
}

static gboolean plugin_unload(GaimPlugin *plugin)
{
	gaim_prefs_disconnect_by_handle(plugin);

	g_list_foreach(gaim_gtk_conv_windows_get_list(), remove_talkfilter_menu, NULL);

	return TRUE;
}

static GaimPluginPrefFrame *
get_plugin_pref_frame(GaimPlugin *plugin)
{
	GaimPluginPrefFrame *frame;
	GaimPluginPref *pref;
	const gtf_filter_t *filter;
	gint ct;

	frame = gaim_plugin_pref_frame_new();

	pref = gaim_plugin_pref_new_with_label(_("Talk Filters"));
	gaim_plugin_pref_frame_add(frame, pref);

	pref = gaim_plugin_pref_new_with_name_and_label(PREF_FILTER, _("Active filter:"));
	gaim_plugin_pref_set_type(pref, GAIM_PLUGIN_PREF_CHOICE);

	gaim_plugin_pref_add_choice(pref, _("(None)"), "");
	ct = filter_count;
	for(filter = filter_list; ct; filter++, ct--)
	{
		gaim_plugin_pref_add_choice(pref, filter->desc, (gpointer)filter->name);
	}

	gaim_plugin_pref_frame_add(frame, pref);

	return frame;
}

static GaimPluginUiInfo prefs_info = {
	get_plugin_pref_frame
};

static GaimPluginInfo talkfilters_info = {
	GAIM_PLUGIN_MAGIC,
	GAIM_MAJOR_VERSION,
	GAIM_MINOR_VERSION,
	GAIM_PLUGIN_STANDARD,								/* type				*/
	GAIM_GTK_PLUGIN_TYPE,								/* ui requirement	*/
	0,													/* flags			*/
	NULL,												/* dependencies		*/
	GAIM_PRIORITY_DEFAULT,								/* priority			*/
	"gtk-plugin_pack-talkfilters",						/* id				*/
	NULL,												/* name				*/
	GPP_VERSION,
	NULL,												/* summary			*/
	NULL,												/* description		*/
	"Mark Lindner <markl@gnu.org>, "
	"Peter Lawler <bleeter@users.sf.net>",
	GPP_WEBSITE,
	plugin_load,
	plugin_unload,
	NULL,

	NULL,
	NULL,
	&prefs_info,
	NULL
};

static void init_plugin(GaimPlugin *plugin) {
	gaim_prefs_add_none("/plugins/gtk/bleeter");
	gaim_prefs_add_none("/plugins/gtk/bleeter/talkfilters");
	gaim_prefs_add_bool("/plugins/gtk/bleeter/talkfilters/enabled", FALSE);
	gaim_prefs_add_string("/plugins/gtk/bleeter/talkfilters/filter", "");
	talkfilters_info.name = _("GNU Talk Filters");
	talkfilters_info.summary = 
		_("Translates text in outgoing messages into humorous dialects.");
	talkfilters_info.description = 
		_("The GNU Talk Filters are filter programs that convert ordinary "
		  "English text into text that mimics a stereotyped or otherwise "
		  "humorous dialect. These filters have been in the public domain for "
		  "many years, and have been made available as a single integrated "
		  "package. The filters include austro, b1ff, brooklyn, chef, cockney, "
		  "drawl, dubya, fudd, funetak, jethro, jive, kraut, pansy, pirate, "
		  "postmodern, redneck, valspeak, and warez.");
}

GAIM_INIT_PLUGIN(talkfilters, init_plugin, talkfilters_info)

/*
 * Hides the blist on signon (or when it's created)
 * Copyright (C) 2004 Gary Kramlich.
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
 */
#ifdef HAVE_CONFIG_H
# include "../pp_config.h"
#endif

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#define PURPLE_PLUGINS

#include <gtkplugin.h>
#include <gtkblist.h>
#include <pluginpref.h>
#include <prefs.h>
#include <version.h>

#include <string.h>

#include "../common/i18n.h"

#define PREF_MY "/plugins/gtk/amc_grim"
#define PREF_ROOT "/plugins/gtk/amc_grim/blistops"
#define PREF_LIST "/plugins/gtk/amc_grim/blistops/hidelist"
#define PREF_MENU "/plugins/gtk/amc_grim/blistops/hidemenu"
#define PREF_STRETCH "/plugins/gtk/amc_grim/blistops/stretch"
#define PREF_EMAIL "/plugins/gtk/amc_grim/blistops/email"

static GtkWidget *w_blist = NULL;
static GtkWidget *w_menubar = NULL;

static void
abracadabra(GtkWidget *w, gboolean v)
{
	if(v)
		gtk_widget_hide(w);
	else
		gtk_widget_show(w);
}

static gboolean
motion_notify_cb(GtkTreeView *view, GdkEventMotion *event, GtkRequisition *req)
{
	if (event->y < req->height) {
		gtk_widget_show(w_menubar);
	} else {
		gtk_widget_hide(w_menubar);
	}

	return FALSE;
}

static void
pref_cb(const char *name, PurplePrefType type,
		gconstpointer val, gpointer data)
{
	static GtkRequisition req;
	gboolean value = GPOINTER_TO_INT(val);

	if(!g_ascii_strcasecmp(name, PREF_LIST))
		abracadabra(w_blist, value);
	else if(!g_ascii_strcasecmp(name, PREF_MENU)) {
		PidginBuddyList *gtkblist = pidgin_blist_get_default_gtk_blist();
		abracadabra(w_menubar, value);
		if (!value)
			g_signal_handlers_disconnect_matched(G_OBJECT(gtkblist->treeview), G_SIGNAL_MATCH_FUNC,
						0, 0, NULL, G_CALLBACK(motion_notify_cb), NULL);
		else {
			gtk_widget_size_request(w_menubar, &req);
			g_signal_connect(gtkblist->treeview, "motion-notify-event", G_CALLBACK(motion_notify_cb), &req);
		}
	}
}

static void
redraw_blist(const char *name, PurplePrefType type,
		gconstpointer val, gpointer data)
{
	pidgin_blist_refresh(data);
}

static void
row_changed_cb(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, PidginBuddyList *gtkblist)
{
	PurpleBlistNode *node;
	gboolean stretch, email;
	static gboolean inuse = FALSE, vis;
	const char *name;
	GdkPixbuf *pix = NULL;
	char *html;

	if (inuse)
		return;

	stretch = purple_prefs_get_bool(PREF_STRETCH);
	email = purple_prefs_get_bool(PREF_EMAIL);
	if (!stretch && !email)
		return;

	inuse = TRUE;

	gtk_tree_model_get(model, iter, NODE_COLUMN, &node,
			BUDDY_ICON_COLUMN, &pix, NAME_COLUMN, &html,
			BUDDY_ICON_VISIBLE_COLUMN, &vis, -1);
	if (PURPLE_BLIST_NODE_IS_CONTACT(node))
		node = (PurpleBlistNode*)purple_contact_get_priority_buddy((PurpleContact*)node);
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node))
		goto end;

	if (email) {
		char *alias = g_markup_escape_text(purple_buddy_get_alias((PurpleBuddy*)node), -1);
		name = purple_buddy_get_name((PurpleBuddy*)node);
		if (g_utf8_collate(alias, name)) {
			char *new;
			char *find = g_strstr_len(html, -1, alias);
			if (find) {
				*find = '\0';
				new = g_strdup_printf("%s%s%s", html, name, find + strlen(alias));
				gtk_tree_store_set(GTK_TREE_STORE(model), iter, NAME_COLUMN, new, -1);
				g_free(new);
			}
		}
		g_free(alias);
	}
	if (stretch && vis && (!pix || pix == gtkblist->empty_avatar)) {
		/* Hiding the empty avatar makes the rows shorter than rows with buddy icons.
		 * We need to find a way to make sure that doesn't happen.
		 */
		gtk_tree_store_set(GTK_TREE_STORE(model), iter, BUDDY_ICON_VISIBLE_COLUMN, FALSE, -1);
		/*g_object_set(gtkblist->text_rend, "height", 32, NULL);*/
	}

end:
	if (pix)
		g_object_unref(pix);
	g_free(html);
	inuse = FALSE;
}

static void
gtkblist_created_cb(PurpleBuddyList *blist)
{
	PidginBuddyList *gtkblist = PIDGIN_BLIST(blist);

	w_blist = gtkblist->window;
	w_menubar = gtk_item_factory_get_widget(gtkblist->ift, "<PurpleMain>");

	g_signal_connect(gtkblist->treemodel, "row_changed", G_CALLBACK(row_changed_cb), gtkblist);

	purple_prefs_trigger_callback(PREF_LIST);
	purple_prefs_trigger_callback(PREF_MENU);
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	purple_signal_connect(pidgin_blist_get_handle(), "gtkblist-created", plugin,
						PURPLE_CALLBACK(gtkblist_created_cb), NULL);

	if (pidgin_blist_get_default_gtk_blist())
		gtkblist_created_cb(purple_get_blist());
	purple_prefs_connect_callback(plugin, PREF_LIST, pref_cb, NULL);
	purple_prefs_connect_callback(plugin, PREF_MENU, pref_cb, NULL);

	purple_prefs_connect_callback(plugin, PREF_STRETCH, redraw_blist, purple_get_blist());
	purple_prefs_connect_callback(plugin, PREF_EMAIL, redraw_blist, purple_get_blist());

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	if(w_blist) {
		gtk_widget_show(w_blist);

		if(w_menubar)
			gtk_widget_show(w_menubar);
	}

	return TRUE;
}

static PurplePluginPrefFrame *
get_plugin_pref_frame(PurplePlugin *plugin)
{
	PurplePluginPrefFrame *frame;
	PurplePluginPref *ppref;

	frame = purple_plugin_pref_frame_new();

	ppref = purple_plugin_pref_new_with_name_and_label(PREF_LIST,									
									_("Hide the buddy list when it is created"));
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label(PREF_MENU,
									_("Hide the menu in the buddy list window"));
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label(PREF_STRETCH,
									_("Stretch the buddyname if the buddy has no buddyicon."));
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label(PREF_EMAIL,
									_("Show email addresses for all the buddies."));
	purple_plugin_pref_frame_add(frame, ppref);

	return frame;
}

static PurplePluginUiInfo prefs_info = {
	get_plugin_pref_frame,
	0,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	PIDGIN_PLUGIN_TYPE,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,

	"gtk-plugin_pack-blistops",
	NULL,
	PP_VERSION,
	NULL,
	NULL,
	"Gary Kramlich <grim@reaperworld.com>",
	PP_WEBSITE,

	plugin_load,
	plugin_unload,
	NULL,

	NULL,
	NULL,
	&prefs_info,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Buddy List Options");
	info.summary = _("Gives extended options to the buddy list");
	info.description = _("Gives extended options to the buddy list");

	purple_prefs_add_none(PREF_MY);
	purple_prefs_add_none(PREF_ROOT);
	purple_prefs_add_bool(PREF_LIST, FALSE);
	purple_prefs_add_bool(PREF_MENU, FALSE);
	purple_prefs_add_bool(PREF_STRETCH, TRUE);
	purple_prefs_add_bool(PREF_EMAIL, FALSE);
}

PURPLE_INIT_PLUGIN(blistops, init_plugin, info)

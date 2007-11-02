/*
 * Infopane - Use different views for the details information in conversation windows.
 * Copyright (C) 2007
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

#include "../common/pp_internal.h"

#include "pidgin.h"

#include "conversation.h"
#include "debug.h"
#include "log.h"
#include "notify.h"
#include "prefs.h"
#include "signals.h"
#include "util.h"

#include "gtkconv.h"
#include "gtkimhtml.h"
#include "gtkplugin.h"

#define PLUGIN_ID "gtk-plugin_pack-infopane"

#define PREF_PREFIX "/plugins/gtk/infopane"
#define PREF_POSITION PREF_PREFIX "/position"
#define PREF_DRAG     PREF_PREFIX "/drag"
#define PREF_SINGLE   PREF_PREFIX "/single"
#define PREF_ICON     PREF_PREFIX "/icon"

static gboolean ensure_tabs_are_showing(PurpleConversation *conv)
{
	PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);
	PidginWindow *win = gtkconv->win;
	if (win->gtkconvs && win->gtkconvs->next)
		return FALSE;
	if (purple_prefs_get_bool(PREF_SINGLE)) {
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(win->notebook), TRUE);
	} else if (win->gtkconvs->next == NULL) {
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(win->notebook), FALSE);
	}
	return FALSE;
}

static void set_conv_window_prefs(PidginConversation *gtkconv)
{
	GtkWidget *paned, *vbox;
	GList *list;
	char pref;

	pref = purple_prefs_get_string(PREF_POSITION)[0];

	if (pref == 't')
		goto end_position;

	if (pref == 'n') {
		gtk_widget_hide_all(gtkconv->infopane_hbox->parent);
		goto end_position;
	}

	list = gtk_container_get_children(GTK_CONTAINER(gtkconv->tab_cont));
	paned = list->data;
	vbox = gtk_paned_get_child1(GTK_PANED(paned));

	g_object_ref(G_OBJECT(gtkconv->infopane_hbox->parent));
	gtk_container_remove(GTK_CONTAINER(vbox), gtkconv->infopane_hbox->parent);
	gtk_box_pack_end(GTK_BOX(vbox), gtkconv->infopane_hbox->parent, FALSE, FALSE, 0);
	g_object_unref(G_OBJECT(gtkconv->infopane_hbox->parent));

end_position:
	/* PREF_DRAG */
	/* To disable dragging, setup a listener for button_press_event and return TRUE if the
	 * click is not going to popup up the sendto or the context menu */

	/* PREF_SINGLE */
	ensure_tabs_are_showing(gtkconv->active_conv);

	/* PREF_ICON */
	if (purple_prefs_get_bool(PREF_ICON)) {
		gtk_widget_show(gtkconv->icon);
	} else {
		gtk_widget_hide(gtkconv->icon);
	}

	return;
}

#if 0
static void setup_callback(PurpleConversation *conv, gboolean (*callback)(gpointer data))
{
	if (callback(conv))
		g_timeout_add(1000, callback, conv);
}

static void conversation_deleted(PurpleConversation *conv)
{
	PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);
	PidginWindow *win = gtkconv->win;
	if (win->gtkconvs->next) {
		PidginConversation *p = win->gtkconvs->data;
		int id;
		if (p == gtkconv)
			p = win->gtkconvs->next->data;
		id = g_timeout_add(0, (GSourceFunc)ensure_tabs_are_showing, p->active_conv);
		g_signal_connect_swapped(G_OBJECT(win->window), "destroy",
				G_CALLBACK(g_source_remove), GINT_TO_POINTER(id));
	}
}
#endif

static void
call_ensure_tabs_are_showing(PurpleConversation *conv)
{
	g_timeout_add(0, (GSourceFunc)ensure_tabs_are_showing, conv);
}

static void
pref_changed(gpointer data, ...)
{
	GList *wins = pidgin_conv_windows_get_list();
	for (; wins; wins = wins->next) {
		GList *tabs = pidgin_conv_window_get_gtkconvs(wins->data);
		for (; tabs; tabs = tabs->next) {
			set_conv_window_prefs(tabs->data);
		}
	}
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	guint regsuccess = 0;

	regsuccess = purple_signal_connect(pidgin_conversations_get_handle(),
			"conversation-displayed",
			plugin, PURPLE_CALLBACK(set_conv_window_prefs), NULL);

	if(regsuccess == 0) {
		purple_debug_error(PLUGIN_ID, "Libpurple and Pidgin are too old!\n");
		purple_debug_error(PLUGIN_ID, _("Libpurple and Pidgin are too old!\n"));
		purple_notify_error(plugin, _("Incompatible Plugin"),
				_("You need to update Pidgin!"),
				_("This plugin is incompatible with the running version of Pidgin and Libpurple because it is too old.  Please upgrade to the newest version of Pidgin."));
		return FALSE;
	}

#if 0
	purple_signal_connect(purple_conversations_get_handle(),
			"deleting-conversation",
			plugin, PURPLE_CALLBACK(conversation_deleted), NULL);
#endif
	purple_signal_connect(pidgin_conversations_get_handle(),
			"conversation-switched",
			plugin, PURPLE_CALLBACK(call_ensure_tabs_are_showing), NULL);

	purple_prefs_connect_callback(plugin, PREF_POSITION, (PurplePrefCallback)pref_changed, NULL);
	purple_prefs_connect_callback(plugin, PREF_DRAG, (PurplePrefCallback)pref_changed, NULL);
	purple_prefs_connect_callback(plugin, PREF_ICON, (PurplePrefCallback)pref_changed, NULL);
	purple_prefs_connect_callback(plugin, PREF_SINGLE, (PurplePrefCallback)pref_changed, NULL);

	purple_prefs_trigger_callback(PREF_POSITION);

	return TRUE;
}

static PurplePluginPrefFrame *
get_plugin_pref_frame(PurplePlugin *plugin)
{
	PurplePluginPrefFrame *frame;
	PurplePluginPref *pref;

	frame = purple_plugin_pref_frame_new();

	/* XXX: Is there a better way than this? There really should be. */
	pref = purple_plugin_pref_new_with_name_and_label(PREF_POSITION, _("Position of the infopane ('top', 'bottom' or 'none')"));
	purple_plugin_pref_frame_add(frame, pref);
#if 0
	pref = purple_plugin_pref_new_with_name_and_label(PREF_ICON,
					_("Show icon in the tabs"));
	purple_plugin_pref_frame_add(frame, pref);
#endif
	pref = purple_plugin_pref_new_with_name_and_label(PREF_SINGLE,
					  _("Always show the tab"));
	purple_plugin_pref_frame_add(frame, pref);

	return frame;
}

static PurplePluginUiInfo prefs_info =
{
	get_plugin_pref_frame,
	0,
	NULL,

	/* padding */
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
	PLUGIN_ID,
	NULL,
	VERSION,
	NULL,
	NULL,
	"Sadrul H Chowdhury <sadrul@pidgin.im>",
	PP_WEBSITE,
	plugin_load,
	NULL,
	NULL,
	NULL,
	NULL,
	&prefs_info,
	NULL,

	/* padding */
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

	if(purple_version_check(2,2,0)  == NULL) {
		info.name = _("Infopane Options");
		info.summary = _("Allow customizing the details information in conversation windows.");
		info.description = _("Allow customizing the details information in conversation windows.");

		purple_prefs_add_none(PREF_PREFIX);
		purple_prefs_add_string(PREF_POSITION, "top");
		purple_prefs_add_bool(PREF_DRAG, FALSE);
		purple_prefs_add_bool(PREF_SINGLE, TRUE);
		purple_prefs_add_bool(PREF_ICON, TRUE);
	} else {
		purple_debug_error(PLUGIN_ID, "Libpurple and Pidgin are too old!\n");
		purple_debug_error(PLUGIN_ID, _("Libpurple and Pidgin are too old!\n"));

		info.name = _("Incompatible Plugin! - Check plugin details!");
		info.summary = _("This plugin is NOT compatible with this version of Pidgin!");
		info.description = _("This plugin is NOT compatible with this version of Pidgin!");
	}
}

PURPLE_INIT_PLUGIN(infopane, init_plugin, info)

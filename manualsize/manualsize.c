/*
 *
 * Pidgin 2.4 manual entry area height sizing plugin
 * License: GPL version 2 or later
 *
 * Copyright (C) 2008, Artemy Kapitula <dalt74@gmail.com>
 *
 */

#include "../common/pp_internal.h"

#include <pidgin.h>
#include <gtkprefs.h>

#include <conversation.h>
#include <prefs.h>
#include <signals.h>
#include <version.h>
#include <debug.h>

#include <gtkplugin.h>
#include <gtkutils.h>
#include <gtkimhtml.h>

#define NOTIFY_PLUGIN_ID "pidgin-entry-manual-height"

#define PREF_PREFIX	                       "/plugins/manualsize"
#define PREF_CHAT_ENTRY_HEIGHT PREF_PREFIX "/chat_entry_height"
#define PREF_IM_ENTRY_HEIGHT   PREF_PREFIX "/im_entry_height"

static gboolean page_added = FALSE;    // The flag of page has been added.
    // It's used to track a case when we add a second page and should to do some
    // additional work to track a page resize issues

static GList * books_connected = NULL;
    // List of notebooks we connected to. When plugin is unloaded,
    // we will disconnect our handler for a "page-added" signal

/*
 * Find a first "placed" objects (the object that has allocation with a height > 1)
 * and it's internal height.
 *
 * It's required because when creating a non-first page in the notebook,
 * the widget of the added page has allocation->heigth = 1, and we cannot
 * use it as a base for evaluating position of separator in a GtkVPaned
 */
static GtkWidget *
find_placed_object(GtkWidget * w, int * client_height) {
        GtkWidget * ret;
        gint border_width;
        border_width = gtk_container_get_border_width( GTK_CONTAINER(w) );
        if ((w->allocation.height > 1)||(gtk_widget_get_parent(w)==NULL)) {
                *client_height = w->allocation.height;
                return w;
        } else {
                ret = find_placed_object( gtk_widget_get_parent(w), client_height );
                *client_height = *client_height - border_width + 2;
                return ret;
        }
}

/*
 * Find a GtkNotebook in the widget's parents
 * It's used to find a GtkNotebook in a conversation window
 * to attach a "page-added" signal handler
 */
static GtkWidget *
get_notebook(GtkWidget * w) {
	if (strcmp(GTK_OBJECT_TYPE_NAME(w),"GtkNotebook")==0) return w;
	if (gtk_widget_get_parent(w)==NULL) return NULL;
	return get_notebook(gtk_widget_get_parent(w));
}

/*
 * Signal handler. Triggers a page_added flag.
 */
static void
on_page_add( GtkNotebook * book, GtkWidget * widget, guint page_num, gpointer user_data ) {
	page_added = TRUE;
	return;
}

/*
 * When removing last page, forget this notebook
 */
static void
on_page_remove( GtkNotebook * book, GtkWidget * widget, guint page_num, gpointer user_data ) {
	if (gtk_notebook_get_n_pages(book) == 0) {
		books_connected = g_list_remove( books_connected, book );
		printf("Removed!\n");
	}
}

/*
 * Attach a handlers on a notebook if it is not already attached
 * Adds a notebook into a tracked objects list
 */
static void
connect_notebook_handler(GtkNotebook * notebook) {
	GList * item = g_list_find( books_connected, notebook );
	if (!item) {
		g_signal_connect_after(notebook, "page-added",
		                       G_CALLBACK(on_page_add), NULL);
		g_signal_connect_after(notebook, "page-removed",
		                       G_CALLBACK(on_page_remove), NULL);
		books_connected = g_list_append( books_connected, notebook );
		printf("Added!\n");
	}
}

/*
 * Rebuild conversation pane.
 * Find a conversation pane ("pane")
 * Find a parent for a pane ("top")
 * Create GtkVPaned ("vpaned")
 * Move "pane" from a "top" to the up of "vpaned"
 * Move "lower_hbox" of conversation to the bottom "vpaned"
 * Insert "vpaned" into a "top"
 * Change "vpaned" divider position
 */
static void
rebuild_container(PidginConversation * conv) {

	GtkWidget * pane = gtk_widget_get_parent(GTK_WIDGET(conv->lower_hbox));
	GtkWidget * top = gtk_widget_get_parent( pane );
	GtkWidget * vpaned = gtk_vpaned_new();
	GtkNotebook * notebook = GTK_NOTEBOOK(get_notebook(top));
	gint handle_size = 0;
	gint parent_area = 0;
	gint border_size = 0;
	gint new_pos;
	GtkPositionType tabpos = -1;
	GValue v = {0, };
	gint stored_height = 0;
	
	if(purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT)
		stored_height = purple_prefs_get_int(PREF_CHAT_ENTRY_HEIGHT);
	else
		stored_height = purple_prefs_get_int(PREF_IM_ENTRY_HEIGHT);

	if (stored_height < 0) stored_height = 128;

	if (notebook) {
		tabpos = gtk_notebook_get_tab_pos( notebook );
		connect_notebook_handler( notebook );
	}

	g_value_init( &v, G_TYPE_BOOLEAN );
	
	gtk_widget_show( vpaned );

	g_value_set_boolean( &v, TRUE );
	gtk_widget_reparent( pane, vpaned );
	gtk_container_child_set_property( GTK_CONTAINER(vpaned), pane, "resize", &v );

	g_value_set_boolean( &v, FALSE );
	gtk_widget_reparent( conv->lower_hbox, vpaned );
	gtk_container_child_set_property( GTK_CONTAINER(vpaned), conv->lower_hbox, "resize", &v );

	g_value_unset( &v );

	gtk_container_add( GTK_CONTAINER(top), vpaned );

	gtk_widget_style_get( vpaned, "handle-size", &handle_size, NULL );

	find_placed_object( top, &parent_area );
	border_size = gtk_container_get_border_width(GTK_CONTAINER(top));

	new_pos =
		parent_area -
		stored_height -
		handle_size - 
		border_size * 2 - 
		(((page_added==TRUE)&&((tabpos==GTK_POS_TOP)||(tabpos==GTK_POS_BOTTOM)))?24:0);

	gtk_paned_set_position( GTK_PANED(vpaned), new_pos );

	page_added = FALSE;

	gtk_widget_grab_focus( conv->entry );

}

/*
 * Store input area size depending on a conversation type
 */
static void
store_area_size(PidginConversation * conv) {

	gboolean chat = (conv->active_conv->type == PURPLE_CONV_TYPE_CHAT);
	
	if (strcmp("GtkVPaned",(GTK_OBJECT_TYPE_NAME(gtk_widget_get_parent( GTK_WIDGET(conv->lower_hbox)))))==0) {
		if (chat) {
			purple_prefs_set_int(PREF_CHAT_ENTRY_HEIGHT,
			                     conv->lower_hbox->allocation.height);
		} else {
			purple_prefs_set_int(PREF_IM_ENTRY_HEIGHT,
			                     conv->lower_hbox->allocation.height);
		}
	}
	return;
}

/*
 * Signal handler. Called when conversation created, and rebuilds a conversation pane
 */
static void
on_display(void* data) {
	PidginConversation * gtkconv = (PidginConversation*)data;
	rebuild_container( gtkconv );
}

/*
 * Signal handler. Called when conversation destroyed, to store an input area size
 */
static void
on_destroy(void * data) {
	PurpleConversation * conv = (PurpleConversation*)data;
	PidginConversation * gtkconv;
	if (conv) {
    	        gtkconv = PIDGIN_CONVERSATION( conv );
	        if (gtkconv) {
			store_area_size( gtkconv );
		}
	}
}

/*
 * Traverse connected notebooks and remove our signal handler
 */
static void
cleanup_callback(gpointer data, gpointer user_data) {
	g_signal_handlers_disconnect_by_func( data, on_page_add, NULL );
	g_signal_handlers_disconnect_by_func( data, on_page_remove, NULL );
}

static gboolean
plugin_load(PurplePlugin *plugin) {
	void * gtk_conv_handle = pidgin_conversations_get_handle();
	void * conv_handle = purple_conversations_get_handle();

	purple_prefs_add_none(PREF_PREFIX);
	purple_prefs_add_int(PREF_CHAT_ENTRY_HEIGHT, 128);
	purple_prefs_add_int(PREF_IM_ENTRY_HEIGHT, 128);

	purple_signal_connect(gtk_conv_handle, "conversation-displayed", plugin,
	                    PURPLE_CALLBACK(on_display), NULL);
	purple_signal_connect(conv_handle, "deleting-conversation", plugin,
	                    PURPLE_CALLBACK(on_destroy), NULL);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin) {
	g_list_foreach( books_connected, cleanup_callback, NULL );
	g_list_free( books_connected );
	return TRUE;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,                           /**< type           */
	PIDGIN_PLUGIN_TYPE,                               /**< ui_requirement */
	0,                                                /**< flags          */
	NULL,                                             /**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,                          /**< priority       */

	NOTIFY_PLUGIN_ID,                                 /**< id             */
	NULL,                                             /**< name           */
	PP_VERSION,                                       /**< version        */
	                                                  
	NULL,                                             /**  summary        */
	NULL,                                             /**  description    */
	                                                  
	"Artemy Kapitula <dalt74@gmail.com>",             /**< author         */
	PP_WEBSITE,                                       /**< homepage       */

	plugin_load,                                      /**< load           */
	plugin_unload,                                    /**< unload         */
	NULL,                                             /**< destroy        */

	NULL,                                             /**< ui_info        */
	NULL,                                             /**< extra_info     */
	NULL,
	NULL,

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Entry area manual sizing");
	info.summary = _("Allows you to change entry area height");
	info.description = info.summary;
}

PURPLE_INIT_PLUGIN(manualsize, init_plugin, info)

/*
 * Stocker - Adds a stock ticker to the buddy list
 * Copyright (C) 2005 Gary Kramlich <grim@reaperworld.com>
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

#include "stocker_prefs.h"

#include <gtkprefs.h>
#include <gtkutils.h>

/******************************************************************************
 * Structs
 *****************************************************************************/
#define STOCKER_PREFS(obj)	((StockerPrefs *)(obj))

typedef struct {
	GtkWidget *entry;
	GtkWidget *list;
	GtkListStore *symbols;
} StockerPrefs;

/******************************************************************************
 * helpers
 *****************************************************************************/
static void
stocker_prefs_update_list(StockerPrefs *prefs) {
	GtkTreeIter iter;
	GList *l;
	gchar *symbol;

	gtk_list_store_clear(prefs->symbols);

	for(l = purple_prefs_get_string_list(PREF_SYMBOLS); l; l = l->next) {
		symbol = (gchar *)l->data;

		gtk_list_store_append(prefs->symbols, &iter);
		gtk_list_store_set(prefs->symbols, &iter,
						   0, symbol,
						   -1);
	}
}

static gboolean
stocker_prefs_apply_helper(GtkTreeModel *model, GtkTreePath *path,
						   GtkTreeIter *iter, gpointer data)
{
	GList **symbols = (GList **)data;
	gchar *symbol;

	gtk_tree_model_get(model, iter,
					   0, &symbol,
					   -1);
	*symbols = g_list_append(*symbols, symbol);

	return FALSE;
}

static void
stocker_prefs_apply_cb(GtkButton *button, gpointer data) {
	StockerPrefs *prefs = (StockerPrefs *)data;
	GList *symbols = NULL, *l;

	gtk_tree_model_foreach(GTK_TREE_MODEL(prefs->symbols),
						   stocker_prefs_apply_helper, &symbols);

	purple_prefs_set_string_list(PREF_SYMBOLS, symbols);

	for(l = symbols; l; l = l->next)
		g_free(l->data);
	g_list_free(symbols);
}

static void
stocker_prefs_add_cb(GtkButton *button, gpointer data) {
	StockerPrefs *prefs = (StockerPrefs *)data;
	GtkTreeIter iter;
	const gchar *symbol;

	symbol = gtk_entry_get_text(GTK_ENTRY(prefs->entry));
	if(g_utf8_strlen(symbol, -1) <= 0)
		return;

	gtk_list_store_append(prefs->symbols, &iter);
	gtk_list_store_set(prefs->symbols, &iter,
					   0, symbol,
					   -1);
	gtk_entry_set_text(GTK_ENTRY(prefs->entry), "");
}

static void
stocker_prefs_remove_cb(GtkButton *button, gpointer data) {
	StockerPrefs *prefs = (StockerPrefs *)data;
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	gchar *symbol;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(prefs->list));
	if(!gtk_tree_selection_get_selected(sel, NULL, &iter))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(prefs->symbols), &iter,
					   0, &symbol,
					   -1);
	gtk_entry_set_text(GTK_ENTRY(prefs->entry), symbol);
	g_free(symbol);

	gtk_list_store_remove(prefs->symbols, &iter);
}

static void
stocker_prefs_move_up_cb(GtkButton *button, gpointer data) {
	StockerPrefs *prefs = STOCKER_PREFS(data);
	GtkTreeSelection *sel;
	GtkTreeIter siter, diter;
	GtkTreePath *path;
	
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(prefs->list));
	if(!gtk_tree_selection_get_selected(sel, NULL, &siter)) {
		return;
	}

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(prefs->symbols), &siter);
	if(!path)
		return;

	if(!gtk_tree_path_prev(path))
		return;

	if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(prefs->symbols), &diter, path))
	{
		gtk_tree_path_free(path);
		return;
	}

	gtk_tree_path_free(path);

#if GTK_CHECK_VERSION(2,2,0)
	gtk_list_store_swap(prefs->symbols, &siter, &diter);
#else
# warning Someone make me work on gtk < 2.2.0
#endif
}

static void
stocker_prefs_move_down_cb(GtkButton *button, gpointer data) {
	StockerPrefs *prefs = STOCKER_PREFS(data);
	GtkTreeSelection *sel;
	GtkTreeIter siter, diter;
	GtkTreePath *path;
	
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(prefs->list));
	if(!gtk_tree_selection_get_selected(sel, NULL, &siter)) {
		return;
	}

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(prefs->symbols), &siter);
	if(!path)
		return;

	gtk_tree_path_next(path);

	if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(prefs->symbols), &diter, path))
	{
		gtk_tree_path_free(path);
		return;
	}

	gtk_tree_path_free(path);

#if GTK_CHECK_VERSION(2,2,0)
	gtk_list_store_swap(prefs->symbols, &siter, &diter);
#else
# warning Someone make me work on gtk < 2.2.0
#endif
}

static void
stocker_prefs_destroyed(gpointer data) {
	StockerPrefs *prefs = STOCKER_PREFS(data);

	g_object_unref(G_OBJECT(prefs->symbols));

	g_free(prefs);
}

/******************************************************************************
 * api
 *****************************************************************************/
void
stocker_prefs_init(void) {
	GList *def_syms = NULL;

	def_syms = g_list_append(def_syms, "GOOG");
	def_syms = g_list_append(def_syms, "YHOO");
	def_syms = g_list_append(def_syms, "MSFT");
	def_syms = g_list_append(def_syms, "UYE");
	def_syms = g_list_append(def_syms, "TWX");

	purple_prefs_add_none(PREF_MY);
	purple_prefs_add_none(PREF_ROOT);
	purple_prefs_add_string_list(PREF_SYMBOLS, def_syms);
	purple_prefs_add_int(PREF_INTERVAL, 30);
}

GtkWidget *
stocker_prefs_get_frame(PurplePlugin *plugin) {
	StockerPrefs *prefs = g_new0(StockerPrefs, 1);
	GtkWidget *ret, *vbox, *hbox, *box, *frame, *sw, *label, *button;
	GtkSizeGroup *sg;
	GtkTreeViewColumn *col;
	GtkCellRenderer *rend;

	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	ret = gtk_vbox_new(FALSE, 4);
	gtk_container_set_border_width(GTK_CONTAINER(ret), 12);
	g_object_set_data_full(G_OBJECT(ret), "prefs", prefs,
						   stocker_prefs_destroyed);

	/**********************************
	 * symbols frame
	 *********************************/
	frame = pidgin_make_frame(ret, _("Symbols"));

	box = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(frame), box, FALSE, FALSE, 0);

	vbox = gtk_vbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(box), vbox, FALSE, FALSE, 0);

	/* symbol entry */
	hbox = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new(_("Symbol:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_size_group_add_widget(sg, label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
	prefs->entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), prefs->entry, FALSE, FALSE, 0);

	/* symbols list */
	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
								   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

	/* yes we purposely keep a reference.... */
	prefs->symbols = gtk_list_store_new(1, G_TYPE_STRING);
	stocker_prefs_update_list(prefs);

	prefs->list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(prefs->symbols));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(prefs->list), FALSE);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(prefs->list), TRUE);
	gtk_container_add(GTK_CONTAINER(sw), prefs->list);

	rend = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("Symbol", rend,
												   "text", 0,
												   NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(prefs->list), col);

	/* buttons */
	vbox = gtk_vbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(box), vbox, FALSE, FALSE, 0);

	button = gtk_button_new_from_stock(GTK_STOCK_ADD);
	g_signal_connect(G_OBJECT(button), "clicked",
					 G_CALLBACK(stocker_prefs_add_cb), prefs);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	button = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	g_signal_connect(G_OBJECT(button), "clicked",
					 G_CALLBACK(stocker_prefs_remove_cb), prefs);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	button = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
	g_signal_connect(G_OBJECT(button), "clicked",
					 G_CALLBACK(stocker_prefs_move_down_cb), prefs);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	button = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
	g_signal_connect(G_OBJECT(button), "clicked",
					 G_CALLBACK(stocker_prefs_move_up_cb), prefs);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	button = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	g_signal_connect(G_OBJECT(button), "clicked",
					 G_CALLBACK(stocker_prefs_apply_cb), prefs);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	/**********************************
	 * options frame
	 *********************************/
	frame = pidgin_make_frame(ret, _("Options"));

	pidgin_prefs_labeled_spin_button(frame, "Update interval:",
									   PREF_INTERVAL, 1, 1440, sg);

	/* show and return it already! */
	gtk_widget_show_all(ret);

	return ret;
}

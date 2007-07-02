/* SmartEar by Matt Perry
 * Works for purple 2.0.0
 * Plugin to assign different sounds to different buddies and groups.
 * TODO: figure out contact support
 */

//#include "config.h"
#include "internal.h"
#include "pidgin.h"

#include "gtkplugin.h"
#include "gtkutils.h"
#include "version.h"

#include "debug.h"
#include "util.h"
//#include "ui.h"
#include "sound.h"
#include "gtkprefs.h"
#include "gtkblist.h"
#include "signals.h"

#ifndef PURPLE_PLUGINS
#define PURPLE_PLUGINS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

#ifdef _WIN32
#include "win32dep.h"
#endif

#define RCFILE_VERSION 2
#define RC_EMPTY_SOUND " " // must be at least 1 char long for sscanf

#define SMARTEAR_PLUGIN_ID "smartear"
#define SMARTEAR_VERSION VERSION ".1"

#define FIND(s) lookup_widget(config, s)
#define EFIND(s) lookup_widget(edit_win, s)

// an entry in the prefs file to indicate what to play for who and when.
struct smartear_entry {
	char type; // B,G
	char *name;
#define EVENT_M	0
#define EVENT_S	1
#define EVENT_I	2
#define EVENT_A	3
#define EVENT_NUM 4
	char *sound[EVENT_NUM];
};

struct timer_data {
	int event;
	guint id;
};

struct message_data {
	PurpleAccount *account;
	char *buddy;
	time_t last_active;
	time_t last_sent;
	struct timer_data *timer;
};

enum { // Treeview columns
	DATA_COL = 0,
	TYPE_COL = 0,
	NAME_COL,
	ON_MESSAGE_COL,
	ON_SIGNON_COL,
	ON_UNIDLE_COL,
	ON_UNAWAY_COL,
	NUM_COLS
};

#define DEFAULT_NAME "(Default)"
#define IS_DEFAULT(entry) (strcmp(entry->name, DEFAULT_NAME)==0)
#define IS_EMPTY(str) (!str || !str[0])
struct smartear_entry default_entry = {'B', DEFAULT_NAME, {"", "", "", ""}};

int message_delay = 60;
int focused_quiet = TRUE;
int smartear_timers = FALSE;

GtkWidget *config = NULL;
GtkWidget *edit_win = NULL;
GtkWidget *file_browse = NULL;
GtkWidget *file_browse_entry = NULL;
GtkTreeView *treeview = NULL;
GtkTreeSelection *treeselect = NULL;
GtkTreeModel *treemodel = NULL;

struct smartear_entry *selected_entry = NULL;
GtkTreeIter selected_iter;

GSList *sounds_list = NULL;
GSList *messages_list = NULL;

GtkWidget *create_file_browse(void);
GtkWidget *create_edit_win(void);
GtkWidget *create_config(void);

static void new_entry(struct smartear_entry *entry);
static gboolean get_selection(void);
static void clear_selection(void);
static void populate_edit_win(struct smartear_entry *entry);
static void update_list(void);
static void populate_list(GtkListStore *store);
static void list_set(GtkListStore *store, GtkTreeIter *iter, struct smartear_entry *entry);
static void set_option_entries(void);

static gint sound_cmp(gconstpointer p1, gconstpointer p2);
static struct smartear_entry *sound_new_with_name(gchar *name);
static struct smartear_entry *sound_dup(struct smartear_entry *entry);
static void sound_free(struct smartear_entry *entry, int free_entry);
static void soundlist_free(void);

static gchar *get_basename(gchar *path);

static void on_treeselect_changed(GtkTreeSelection *selection, gpointer data);

static void smartear_save(void);
static void smartear_load(void);

static void play_sound_alias(char *sound, PurpleAccount* account);

/*** Glade's Support Function ***/

GtkWidget* lookup_widget (GtkWidget *widget, const gchar *widget_name)
{
	GtkWidget *found_widget;
	found_widget = (GtkWidget*) g_object_get_data (G_OBJECT (widget),
												   widget_name);
	if (!found_widget)
		g_warning ("smartear: Widget not found: %s", widget_name);
	return found_widget;
}

/*** GTK Callbacks ***/

// Options Frame

void on_delay_changed (GtkEditable *editable, gpointer user_data)
{
	gchar *text = gtk_editable_get_chars(editable, 0, -1);
	message_delay = atoi(text);
	g_free(text);
}

void on_focus_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	focused_quiet = gtk_toggle_button_get_active(togglebutton);
}

void on_timer_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	smartear_timers = gtk_toggle_button_get_active(togglebutton);
}

// Entries Frame

void on_cell_edited(GtkCellRendererText *cell, gchar *path, gchar *text, gpointer data)
{
	GtkTreeIter iter;
	gint col = GPOINTER_TO_INT(data);
	struct smartear_entry *entry;

	if (!gtk_tree_model_get_iter_from_string(treemodel, &iter, path))
		return;

	gtk_tree_model_get(treemodel, &iter, DATA_COL, &entry, -1);
	switch (col) {
	case NAME_COL:
		if (IS_DEFAULT(entry))
			return;
		g_free(entry->name);
		entry->name = g_strdup(text);
		update_list();
		break;
	}
}

void on_treeselect_changed(GtkTreeSelection *selection, gpointer data)
{
	get_selection();
	update_list();
}


void on_new_clicked (GtkButton *button, gpointer user_data)
{
	if (edit_win)
		return;

	new_entry(sound_new_with_name(""));
}

void on_delete_clicked (GtkButton *button, gpointer user_data)
{
	GtkTreeIter iter;

	if (edit_win)
		return;

	if (!get_selection())
		return;

	if (IS_DEFAULT(selected_entry))
		return;

	iter = selected_iter;

	sounds_list = g_slist_remove(sounds_list, selected_entry);
	sound_free(selected_entry, TRUE);

	if (!gtk_tree_model_iter_next(treemodel, &iter)) {
		GtkTreePath *path = gtk_tree_model_get_path(treemodel, &selected_iter);
		gtk_tree_path_prev(path);
		gtk_tree_model_get_iter(treemodel, &iter, path);
		gtk_tree_path_free(path);
	}
	g_signal_handlers_block_by_func(G_OBJECT(treeselect), on_treeselect_changed, 0);
	gtk_list_store_remove(GTK_LIST_STORE(treemodel), &selected_iter);
	gtk_tree_selection_select_iter(treeselect, &iter);
	g_signal_handlers_unblock_by_func(G_OBJECT(treeselect), on_treeselect_changed, 0);

	get_selection();
	update_list();
}

void on_edit_clicked (GtkButton *button, gpointer user_data)
{
	if (!get_selection())
		return;

	if (edit_win)
		return;

	edit_win = create_edit_win();
	gtk_widget_show_all(edit_win);

	populate_edit_win(selected_entry);
}

void on_save_clicked (GtkButton *button, gpointer user_data)
{
	smartear_save();
}

void on_revert_clicked (GtkButton *button, gpointer user_data)
{
	selected_entry = NULL;
	if (edit_win) {
		gtk_widget_destroy(edit_win);
		edit_win = NULL;
	}

	soundlist_free();
	smartear_load();
	gtk_list_store_clear(GTK_LIST_STORE(treemodel));
	populate_list(GTK_LIST_STORE(treemodel));
	update_list();
	set_option_entries();
}

// Edit Window

void on_browse_ok_clicked (GtkButton *button, gpointer user_data)
{
	gtk_entry_set_text(GTK_ENTRY(file_browse_entry),
					   gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_browse)));
}

void on_file_browse_destroy (GtkObject *object, gpointer user_data)
{
	file_browse = NULL;
	file_browse_entry = NULL;
}

void on_browse_clicked (GtkButton *button, gchar *user_data)
{
	gchar *text;
	if (file_browse)
		return;

	file_browse = create_file_browse();
	gtk_widget_show(file_browse);

	file_browse_entry = EFIND(user_data);
	text = gtk_editable_get_chars(GTK_EDITABLE(file_browse_entry), 0, -1);
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_browse), text);
	g_free(text);
}

void on_im_browse_clicked (GtkButton *button, gpointer user_data)
{
	on_browse_clicked(button, "im_sound_entry");
}

void on_signon_browse_clicked (GtkButton *button, gpointer user_data)
{
	on_browse_clicked(button, "signon_sound_entry");
}

void on_unidle_browse_clicked (GtkButton *button, gpointer user_data)
{
	on_browse_clicked(button, "unidle_sound_entry");
}

void on_unaway_browse_clicked (GtkButton *button, gpointer user_data)
{
	on_browse_clicked(button, "unaway_sound_entry");
}

void on_test_clicked (GtkButton *button, gchar *user_data)
{
	gchar *text =
		gtk_editable_get_chars(GTK_EDITABLE(EFIND(user_data)), 0, -1);
	play_sound_alias(text, NULL);
	g_free(text);
}

void on_im_test_clicked (GtkButton *button, gpointer user_data)
{
	on_test_clicked(button, "im_sound_entry");
}

void on_signon_test_clicked (GtkButton *button, gpointer user_data)
{
	on_test_clicked(button, "signon_sound_entry");
}

void on_unidle_test_clicked (GtkButton *button, gpointer user_data)
{
	on_test_clicked(button, "unidle_sound_entry");
}

void on_unaway_test_clicked (GtkButton *button, gpointer user_data)
{
	on_test_clicked(button, "unaway_sound_entry");
}

void on_apply_clicked (GtkButton *button, gpointer user_data)
{
	struct smartear_entry *entry, tmp;

	if (get_selection()) {
		entry = selected_entry;

		g_free(entry->name);
		g_free(entry->sound[0]);
		g_free(entry->sound[1]);
		g_free(entry->sound[2]);
		g_free(entry->sound[3]);
	}
	else {
		entry = &tmp;
	}

	entry->name =
		gtk_editable_get_chars(GTK_EDITABLE(EFIND("name_entry")), 0, -1);
	entry->type =
		gtk_option_menu_get_history(GTK_OPTION_MENU(EFIND("type_option"))) == 0
		? 'B' : 'G';
	entry->sound[EVENT_M] =
		gtk_editable_get_chars(GTK_EDITABLE(EFIND("im_sound_entry")), 0, -1);
	entry->sound[EVENT_S] =
		gtk_editable_get_chars(GTK_EDITABLE(EFIND("signon_sound_entry")), 0, -1);
	entry->sound[EVENT_I] =
		gtk_editable_get_chars(GTK_EDITABLE(EFIND("unidle_sound_entry")), 0, -1);
	entry->sound[EVENT_A] =
		gtk_editable_get_chars(GTK_EDITABLE(EFIND("unaway_sound_entry")), 0, -1);

	if (!selected_entry) {
		GSList *lp;
		if ((lp = g_slist_find_custom(sounds_list, entry, (GCompareFunc)sound_cmp))) {
			sound_free((struct smartear_entry*)lp->data, FALSE);
			*(struct smartear_entry*)lp->data = *entry;
		}
		else {
			struct smartear_entry *copy = g_new(struct smartear_entry, 1);
			*copy = *entry;
			new_entry(copy);
		}
	}

	update_list();
}

void on_edit_win_destroy (GtkObject *object, gpointer user_data)
{
	edit_win = NULL;

	if (config) {
		gtk_widget_set_sensitive(GTK_WIDGET(treeview), TRUE);
	}
}

void on_config_destroy (GtkObject *object, gpointer user_data)
{
	config = NULL;
}

#include "smartear-interface.c"

/*** Util Functions ***/

static void new_entry(struct smartear_entry *entry)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkTreeViewColumn *name_column;

	sounds_list = g_slist_append(sounds_list, entry);

	if (!config)
		return;

	if (get_selection())
		gtk_list_store_insert_after(GTK_LIST_STORE(treemodel), &iter, &selected_iter);
	else
		gtk_list_store_append(GTK_LIST_STORE(treemodel), &iter);

	path = gtk_tree_model_get_path(treemodel, &iter);
	list_set(GTK_LIST_STORE(treemodel), &iter, entry);

	g_signal_handlers_block_by_func(G_OBJECT(treeselect), on_treeselect_changed, 0);
	path = gtk_tree_model_get_path(treemodel, &iter);
	name_column = gtk_tree_view_get_column(treeview, NAME_COL);
	if (entry->name && entry->name[0])
		gtk_tree_view_set_cursor(treeview, path, 0, FALSE);
	else {
		gtk_tree_view_set_cursor(treeview, path, name_column, TRUE);
	}
	gtk_tree_path_free(path);
	g_signal_handlers_unblock_by_func(G_OBJECT(treeselect), on_treeselect_changed, 0);

	get_selection();
}

static gboolean get_selection(void)
{
	selected_entry = NULL;

	if (!config)
		return FALSE;

	if (!gtk_tree_selection_get_selected(treeselect, &treemodel, &selected_iter))
		return FALSE;

	gtk_tree_model_get(treemodel, &selected_iter, DATA_COL, &selected_entry, -1);
	return TRUE;
}

void clear_selection(void)
{
	selected_entry = NULL;

	if (!config)
		return;

	if (gtk_tree_selection_get_selected(treeselect, &treemodel, &selected_iter)) {
		gtk_tree_selection_unselect_iter(treeselect, &selected_iter);
	}
}

static void list_set(GtkListStore *store, GtkTreeIter *iter, struct smartear_entry *entry)
{
	gtk_list_store_set(store, iter, DATA_COL, entry, -1);
}

static char *my_normalize(const char *input)
{
	static char buf[BUF_LEN];
	char *str, *beg;
	int i=0;

	g_return_val_if_fail((input != NULL), NULL);

	beg = str = g_strdup(input);

	while (*str && i <= BUF_LEN) {
		if (*str != ' ')
			buf[i++] = *str;
		str++;
	}
	buf[i] = '\0';
	g_free(beg);

	return buf;

}

static struct smartear_entry *sound_new_with_name(gchar *name)
{
	struct smartear_entry tmp = default_entry;
	tmp.name = name;
	return sound_dup(&tmp);
}

static struct smartear_entry *sound_dup(struct smartear_entry *entry)
{
	struct smartear_entry *edup = g_new(struct smartear_entry, 1);
	int i;

	edup->type = entry->type;
	edup->name = g_strdup(my_normalize(entry->name));
	for (i = 0; i < EVENT_NUM; i++) {
		edup->sound[i] = g_strdup(entry->sound[i]);
	}

	return edup;
}

static void sound_free(struct smartear_entry *entry, int free_entry)
{
	g_free(entry->name);
	g_free(entry->sound[0]);
	g_free(entry->sound[1]);
	g_free(entry->sound[2]);
	g_free(entry->sound[3]);
	if (free_entry) {
		g_free(entry);
	}
}

static void soundlist_free(void)
{
	GSList *lp;
	for (lp = sounds_list; lp; lp = g_slist_next(lp))
		sound_free((struct smartear_entry*)lp->data, TRUE);
	if (sounds_list) {
		g_slist_free(sounds_list);
		sounds_list = 0;
	}
}

static gint sound_cmp(gconstpointer p1, gconstpointer p2)
{
	struct smartear_entry *entry1 = (struct smartear_entry*)p1;
	struct smartear_entry *entry2 = (struct smartear_entry*)p2;

	if (!entry1 || IS_DEFAULT(entry1))
		return -1;
	if (!entry2 || IS_DEFAULT(entry2))
		return 1;

	// only compare types if both are nonzero
	if (entry1->type != entry2->type &&
		entry1->type != 0 && entry2->type != 0)
		return (entry1->type - entry2->type);
	return (g_strcasecmp(entry1->name, entry2->name));
}

static void populate_edit_win(struct smartear_entry *entry)
{
	gtk_entry_set_text(GTK_ENTRY(EFIND("name_entry")), entry->name);
	gtk_entry_set_text(GTK_ENTRY(EFIND("im_sound_entry")),
					   entry->sound[EVENT_M]);
	gtk_entry_set_text(GTK_ENTRY(EFIND("signon_sound_entry")),
					   entry->sound[EVENT_S]);
	gtk_entry_set_text(GTK_ENTRY(EFIND("unidle_sound_entry")),
					   entry->sound[EVENT_I]);
	gtk_entry_set_text(GTK_ENTRY(EFIND("unaway_sound_entry")),
					   entry->sound[EVENT_A]);
	gtk_option_menu_set_history(GTK_OPTION_MENU(EFIND("type_option")),
								(entry->type == 'B' ? 0 : 1));

	if (IS_DEFAULT(entry)) {
		gtk_widget_set_sensitive(EFIND("name_entry"), FALSE);
		gtk_widget_set_sensitive(EFIND("type_menu"), FALSE);
	}

	if (config) {
		gtk_widget_set_sensitive(GTK_WIDGET(treeview), FALSE);
	}
}

static void set_option_entries(void)
{
	GtkSpinButton *delay_spin = GTK_SPIN_BUTTON(FIND("delay_spin"));
	GtkToggleButton *focus_but = GTK_TOGGLE_BUTTON(FIND("focus_but"));
	GtkToggleButton *timer_but = GTK_TOGGLE_BUTTON(FIND("timer_but"));

	g_signal_handlers_block_by_func(G_OBJECT(delay_spin), on_delay_changed, 0);
	g_signal_handlers_block_by_func(G_OBJECT(focus_but), on_focus_toggled, 0);
	g_signal_handlers_block_by_func(G_OBJECT(timer_but), on_timer_toggled, 0);

	gtk_spin_button_set_value(delay_spin, (gdouble)message_delay);
	gtk_toggle_button_set_active(focus_but, focused_quiet);
	gtk_toggle_button_set_active(timer_but, smartear_timers);

	g_signal_handlers_unblock_by_func(G_OBJECT(delay_spin), on_delay_changed, 0);
	g_signal_handlers_unblock_by_func(G_OBJECT(focus_but), on_focus_toggled, 0);
	g_signal_handlers_unblock_by_func(G_OBJECT(timer_but), on_timer_toggled, 0);
}

static void update_list(void)
{
	if (selected_entry)
		list_set(GTK_LIST_STORE(treemodel), &selected_iter, selected_entry);
}

static gchar* get_basename(gchar *path)
{
	static gchar base[BUF_LEN];
	gchar *p;

	if (!path || !path[0]) {
		return "";
	}

	if ((p = strrchr(path, '/'))) {
		p++;
	}
	else {
		p = path;
	}

	strncpy(base, p, sizeof(base));

	return base;
}

/*** Init Functions ***/

static void populate_list(GtkListStore *store)
{
	GtkTreeIter iter;
	GSList *lp;

	sounds_list = g_slist_sort(sounds_list, (GCompareFunc)sound_cmp);

	for (lp = sounds_list; lp; lp = g_slist_next(lp)) {
		struct smartear_entry *entry = (struct smartear_entry*)lp->data;
		gtk_list_store_append(store, &iter);
		list_set(store, &iter, entry);
		if (entry == selected_entry)
			selected_iter = iter;
	}
}

static void render_type(GtkTreeViewColumn *column, GtkCellRenderer *cell,
						GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	struct smartear_entry *entry;
	gtk_tree_model_get(model, iter, DATA_COL, &entry, -1);
	if (IS_DEFAULT(entry))
		g_object_set(cell, "text", "", NULL);
	else
		g_object_set(cell, "text", entry->type == 'B' ? "Buddy" : "Group", NULL);
}

static void render_name(GtkTreeViewColumn *column, GtkCellRenderer *cell,
						GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	struct smartear_entry *entry;
	gtk_tree_model_get(model, iter, DATA_COL, &entry, -1);
	g_object_set(cell, "text", entry->name, NULL);
}

static void render_when(GtkTreeViewColumn *column, GtkCellRenderer *cell,
						GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	struct smartear_entry *entry;
	gint which = GPOINTER_TO_INT(data);
	gtk_tree_model_get(model, iter, DATA_COL, &entry, -1);
	g_object_set(cell, "text", get_basename(entry->sound[which]), NULL);
}

static gint list_cmp(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer d)
{
	struct smartear_entry *entry1, *entry2;

	gtk_tree_model_get(model, a, DATA_COL, &entry1, -1);
	gtk_tree_model_get(model, b, DATA_COL, &entry2, -1);

	return sound_cmp(entry1, entry2);
}

static void setup_list(void)
{
	GtkListStore *store;
	GtkCellRenderer *cell;

	treeview = GTK_TREE_VIEW(FIND("treeview"));

	store = gtk_list_store_new(1, G_TYPE_POINTER);
	populate_list(store);

	gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store)); // treeview has it's own copy.
	treemodel = gtk_tree_view_get_model(treeview);
	gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(treemodel),
											(GtkTreeIterCompareFunc)&list_cmp, 0, 0);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(treemodel),
										 GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

	treeselect = gtk_tree_view_get_selection(treeview);
	gtk_tree_selection_set_mode(treeselect, GTK_SELECTION_SINGLE);
	g_signal_connect(G_OBJECT(treeselect), "changed",
					 G_CALLBACK(on_treeselect_changed), NULL);

	// Type column
	cell = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_data_func(treeview, -1, "Type",
											   cell, render_type, 0, 0);

	// Name column
	cell = gtk_cell_renderer_text_new();
	g_signal_connect(G_OBJECT(cell), "edited",
					 G_CALLBACK(on_cell_edited), GINT_TO_POINTER(NAME_COL));
	g_object_set(G_OBJECT(cell), "editable", TRUE, NULL);
	gtk_tree_view_insert_column_with_data_func(treeview, -1, "Name",
											   cell, render_name, 0, 0);

	cell = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_data_func(treeview, -1, "On Message",
											   cell, render_when, GINT_TO_POINTER(EVENT_M), 0);

	cell = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_data_func(treeview, -1, "On Signon",
											   cell, render_when, GINT_TO_POINTER(EVENT_S), 0);

	cell = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_data_func(treeview, -1, "On Unidle",
											   cell, render_when, GINT_TO_POINTER(EVENT_I), 0);

	cell = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_data_func(treeview, -1, "On Unaway",
											   cell, render_when, GINT_TO_POINTER(EVENT_A), 0);
}

static void smartear_save(void)
{
	char file[BUF_LEN] = "smartear.rc - you have no home dir";
	FILE *fp = NULL;
	GSList *lp;
	struct smartear_entry *entry;

	if (purple_user_dir()) {
		g_snprintf(file, sizeof(file), "%s/smartear.rc", purple_user_dir());
		fp = fopen(file, "w");
	}

	if (fp == NULL) {
		g_warning("couldn't open %s\n", file);
		return;
	}

	fprintf(fp, "version %d\n", RCFILE_VERSION);
	fprintf(fp, "smartear_timers %d\n", smartear_timers);
	fprintf(fp, "delay %d\n", message_delay);
	fprintf(fp, "focused_quiet %d\n", focused_quiet);

	// save empties as a single space so scanf doesn't get confused
#define SAVE_STR(str) (IS_EMPTY(str) ? RC_EMPTY_SOUND : str)
	for (lp = sounds_list; lp; lp = g_slist_next(lp)) {
		entry = (struct smartear_entry*)lp->data;
		fprintf(fp, "%c {%s} {%s} {%s} {%s} {%s}\n",
				entry->type, entry->name,
				SAVE_STR(entry->sound[EVENT_M]),
				SAVE_STR(entry->sound[EVENT_S]),
				SAVE_STR(entry->sound[EVENT_I]),
				SAVE_STR(entry->sound[EVENT_A]));
	}

	fclose(fp);
}

static void smartear_load(void)
{
	char file[BUF_LEN] = "smartear.rc - you have no home dir";
	FILE *fp = NULL;
	struct smartear_entry *entry;
	char buf[BUF_LEN];
	gboolean has_default = FALSE;
	int rcfile_version = 1;

	if (purple_user_dir()) {
		g_snprintf(file, sizeof(file), "%s/smartear.rc", purple_user_dir());
		fp = fopen(file, "r");
	}

	if (fp == NULL) {
		g_warning("smartear: couldn't open %s\n", file);
		sounds_list = g_slist_append(sounds_list, sound_dup(&default_entry));
		return;
	}

	while ((fgets(buf, sizeof(buf), fp))) {
		int tmp;
		if (sscanf(buf, "version %d", &tmp) == 1)
			rcfile_version = tmp;
		else if (sscanf(buf, "smarterear_timers %d", &tmp) == 1)
			smartear_timers = tmp;
		else if (sscanf(buf, "smartear_timers %d", &tmp) == 1)
			smartear_timers = tmp;
		else if (sscanf(buf, "delay %d", &tmp) == 1)
			message_delay = tmp;
		else if (sscanf(buf, "focused_quiet %d", &tmp) == 1)
			focused_quiet = tmp;
		else if (rcfile_version == 1) {
			char name[BUF_LEN];
			char flags[BUF_LEN];
			char sound[BUF_LEN];
			char type;

			if (sscanf(buf, "%c {%[^}]} {%[^}]} {%[^}]}",
					   &type, name, flags, sound) == 4) {
				if (type != 'G' && type != 'B') {
					g_warning("smartear: rc no such type: %c", type);
					continue;
				}

				entry = g_new(struct smartear_entry, 1);
				entry->type = type;
				entry->name = g_strdup(my_normalize(name));
				if (strchr(flags, 'M'))	entry->sound[EVENT_M] = g_strdup(sound);
				else entry->sound[EVENT_M] = g_strdup("");
				if (strchr(flags, 'S'))	entry->sound[EVENT_S] = g_strdup(sound);
				else entry->sound[EVENT_S] = g_strdup("");
				if (strchr(flags, 'I'))	entry->sound[EVENT_I] = g_strdup(sound);
				else entry->sound[EVENT_I] = g_strdup("");
				if (strchr(flags, 'A'))	entry->sound[EVENT_A] = g_strdup(sound);
				else entry->sound[EVENT_A] = g_strdup("");

				sounds_list = g_slist_append(sounds_list, entry);

				if (IS_DEFAULT(entry)) {
					default_entry = *entry;
					has_default = TRUE;
				}
			}
			else
				g_warning("smartear: rc1 syntax error: %s", buf);
		}
		else if (rcfile_version == 2) {
			char name[BUF_LEN];
			char sound[EVENT_NUM][BUF_LEN];
			char type;
			int i;

			if (sscanf(buf, "%c {%[^}]} {%[^}]} {%[^}]} {%[^}]} {%[^}]}",
					   &type, name, sound[EVENT_M], sound[EVENT_S],
					   sound[EVENT_I], sound[EVENT_A]) == 6) {
				if (type != 'G' && type != 'B') {
					g_warning("smartear: rc no such type: %c", type);
					continue;
				}

				for (i = 0; i < EVENT_NUM; i++) {
					if (strcmp(sound[i], RC_EMPTY_SOUND) == 0)
						sound[i][0] = 0;
				}

				entry = g_new(struct smartear_entry, 1);
				entry->type = type;
				entry->name = g_strdup(my_normalize(name));
				entry->sound[EVENT_M] = g_strdup(sound[EVENT_M]);
				entry->sound[EVENT_S] = g_strdup(sound[EVENT_S]);
				entry->sound[EVENT_I] = g_strdup(sound[EVENT_I]);
				entry->sound[EVENT_A] = g_strdup(sound[EVENT_A]);

				sounds_list = g_slist_append(sounds_list, entry);

				if (IS_DEFAULT(entry)) {
					default_entry = *entry;
					has_default = TRUE;
				}
			}
			else
				g_warning("smartear: rc2 syntax error: %s", buf);
		}
		else
			g_warning("smartear: rc syntax error: %s", buf);
	}
	if (!has_default)
		sounds_list = g_slist_append(sounds_list, sound_dup(&default_entry));

	fclose(fp);
}

/*** Purple callbacks ***/

static struct message_data *find_message_by_name(PurpleAccount *account, const char *pname)
{
	GSList *lp;
	struct message_data *msg = NULL;
	char *name = g_strdup(my_normalize(pname));

	for (lp = messages_list; lp; lp = g_slist_next(lp)) {
		msg = (struct message_data*)lp->data;
		if (msg->account == account
			&& g_strcasecmp(msg->buddy, name) == 0)
			break;
	}

	g_free(name);

	if (lp)
		return msg;
	return 0;
}

static void message_timer_remove(struct message_data *msg)
{
	if (msg && msg->timer) {
		g_source_remove(msg->timer->id);
		g_free(msg->timer);
		msg->timer = NULL;
	}
}

static void message_free(struct message_data *msg)
{
	message_timer_remove(msg);
	g_free(msg->buddy);
	g_free(msg);
}

static void messagelist_free()
{
	GSList *lp;
	for (lp = messages_list; lp; lp = g_slist_next(lp))
		message_free((struct message_data*)lp->data);
	if (messages_list) {
		g_slist_free(messages_list);
		messages_list = NULL;
	}
}

static void on_smartear_clicked(PurpleBlistNode* node, gpointer data)
{
	struct smartear_entry tmp, *entry;
	GSList *lp;

	clear_selection();

	if (!edit_win) {
		edit_win = create_edit_win();
	}
	gtk_widget_show_all(edit_win);

	tmp = default_entry;

	if (!node) {
		return;
	}
	else if (PURPLE_BLIST_NODE_IS_BUDDY(node)) {
		tmp.type = 'B';
		tmp.name = ((PurpleBuddy*)node)->name;
		purple_debug(PURPLE_DEBUG_INFO, "smartear", "adding buddy %s", tmp.name);
	}
	else if (PURPLE_BLIST_NODE_IS_CONTACT(node)) {
		tmp.type = 'B';
		tmp.name = ((PurpleContact*)node)->alias;
		if (!tmp.name) {
			tmp.name = ((PurpleContact*)node)->priority->name;
		}
		purple_debug(PURPLE_DEBUG_INFO, "smartear", "adding contact %s", tmp.name);
	}
	else if (PURPLE_BLIST_NODE_IS_GROUP(node)) {
		tmp.type = 'G';
		tmp.name = ((PurpleGroup*)node)->name;
		g_warning("group %p, %p %s", node, tmp.name, tmp.name);
		purple_debug(PURPLE_DEBUG_INFO, "smartear", "adding group %s", tmp.name);
	}
	else {
		return;
	}

	if ((lp = g_slist_find_custom(sounds_list, &tmp, (GCompareFunc)sound_cmp))) {
		entry = (struct smartear_entry*)lp->data;
	}
	else {
		entry = &tmp;
	}

	populate_edit_win(entry);
}

static void play_sound_alias(char *sound, PurpleAccount* account)
{
	struct smartear_entry tmp, *entry;
	GSList *lp;

	if (!sound || !*sound)
		return;

	// sound aliases: mostly so you can put (Default) for sound, and it'll
	// play that one
	tmp.type = 0;
	tmp.name = sound;
	if ((lp = g_slist_find_custom(sounds_list, &tmp, (GCompareFunc)sound_cmp))) {
		entry = (struct smartear_entry*)lp->data;
		play_sound_alias(entry->sound[EVENT_M], account);
	}
	else {
		purple_sound_play_file(sound, account);
	}
}

void play_matching_sound(PurpleBuddy *buddy, int event)
{
	GSList *lp;
	struct smartear_entry *entry;
	char *sound = NULL;
	char *name = buddy ? g_strdup(my_normalize(buddy->name)) : NULL;
	PurpleGroup *g = buddy ? purple_buddy_get_group(buddy) : NULL;

	for (lp = sounds_list; lp; lp = g_slist_next(lp)) {
		entry = (struct smartear_entry*)lp->data;
		if (!entry->sound[event])
			continue;

		if (entry->type == 'B' && name
			&& g_strcasecmp(name, entry->name) == 0) {
			sound = entry->sound[event];
			// found a buddy match.. this takes precedence, so stop
			break;
		}
		if (entry->type == 'G' && g
			&& g_strcasecmp(my_normalize(g->name), entry->name) == 0) {
			sound = entry->sound[event];
			// keep going... buddy overrides group
		}
		if (IS_DEFAULT(entry)) {
			if (!sound)
				sound = entry->sound[event];
			// keep going.. other 2 override default
		}
	}
	if (!IS_EMPTY(sound)) {
		purple_debug(PURPLE_DEBUG_INFO, "smartear",
				   "found %s for %s on event %d\n", sound, name, event);
		play_sound_alias(sound, purple_buddy_get_account(buddy));
	}
	else {
		purple_debug(PURPLE_DEBUG_INFO, "smartear",
				   "no sound found for %s on event %d\n", name, event);
	}
	g_free(name);
}

static gint play_sound_timer_hook(gpointer data)
{
	struct message_data *msg = (struct message_data *)data;

	if(!msg || !msg->timer) {
		g_warning("smartear: timer called without being active!\n");
		return FALSE;
	}

	play_matching_sound(purple_find_buddy(msg->account, msg->buddy), msg->timer->event);

	g_free(msg->timer);
	msg->timer = NULL;

	return FALSE;
}

static gboolean on_im_recv(PurpleAccount *account, char *who, char *what, gint32 flags, void *junk)
{
	struct message_data *msg;
	time_t now = time(0);
	PurpleConversation *conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_ANY, who, account);
	PidginConversation *gtkconv = conv ? PIDGIN_CONVERSATION(conv) : NULL;

	// FIXME: message list is never trimmed. However, if we DO trim it,
	// we need to consider race conditions on the msg field of the timer_hook

	if ((msg = find_message_by_name(account, who))) {
		// only install a timer if there's no active timers and we've just
		// sent a message. If we haven't just sent a message, then
		// a beep will go off anyways when the IM arrives.
		if(smartear_timers && !msg->timer
		   && msg->last_sent + message_delay > now) {
			msg->timer = g_new0(struct timer_data, 1);
			msg->timer->event = EVENT_M;
			msg->timer->id = g_timeout_add(message_delay*1000,
										   play_sound_timer_hook, msg);
		}

		if (msg->last_active + message_delay > now && conv) {
			purple_debug(PURPLE_DEBUG_INFO, "smartear",
					   "received IM from %s, but only %d/%d seconds passed.\n",
					   who, now - msg->last_active, message_delay);
			return FALSE;
		}
	}
	else {
		msg = g_new0(struct message_data, 1);
		msg->account = account;
		msg->buddy = g_strdup(my_normalize(who));
		msg->timer = NULL;

		messages_list = g_slist_append(messages_list, msg);
	}

	msg->last_active = now;

	if (focused_quiet && gtkconv && GTK_WIDGET_HAS_FOCUS(gtkconv->entry))
		return FALSE; // don't play sounds for the focused convo
	if (gtkconv && !gtkconv->make_sound)
		return FALSE;

	play_matching_sound(purple_find_buddy(msg->account, msg->buddy), EVENT_M);

	return FALSE;
}

static void on_im_send(PurpleAccount *account, char *who, char *what, void *junk)
{
	struct message_data *msg;
	time_t now = time(0);

	if ((msg = find_message_by_name(account, who))) {
		message_timer_remove(msg);
	}
	else {
		msg = g_new0(struct message_data, 1);
		msg->account = account;
		msg->buddy = g_strdup(my_normalize(who));
		msg->timer = NULL;

		messages_list = g_slist_append(messages_list, msg);
	}

	msg->last_active = now;
	msg->last_sent = now;
}

static void on_buddy_signon(PurpleBuddy *buddy, void *data)
{
	play_matching_sound(buddy, EVENT_S);
}

static void on_buddy_back(PurpleBuddy *buddy, void *data)
{
	play_matching_sound(buddy, EVENT_A);
}

static void on_buddy_unidle(PurpleBuddy *buddy, void *data)
{
	play_matching_sound(buddy, EVENT_I);
}

static void on_signon(PurpleConnection *gc, void *m)
{
}

static void on_signoff(PurpleConnection *gc, void *m)
{
	messagelist_free();
}

static void on_blist_node_extended_menu(PurpleBlistNode *node, GList **menu)
{
    PurpleMenuAction *menu_action;
    menu_action = purple_menu_action_new(_("Edit SmartEar Entry"),
            PURPLE_CALLBACK(on_smartear_clicked), NULL, NULL);
    *menu = g_list_append(*menu, menu_action);
}

static gboolean purple_plugin_init(PurplePlugin *plugin)
{
	void *blist_handle = purple_blist_get_handle();
	void *conv_handle = purple_conversations_get_handle();

	config = NULL;

	smartear_load();

	purple_signal_connect(blist_handle, "blist-node-extended-menu",
					   	plugin, PURPLE_CALLBACK(on_blist_node_extended_menu), NULL);

	purple_signal_connect(blist_handle, "buddy-signed-on",
						plugin, PURPLE_CALLBACK(on_buddy_signon), NULL);
	purple_signal_connect(blist_handle, "buddy-back",
						plugin, PURPLE_CALLBACK(on_buddy_back), NULL);
	purple_signal_connect(blist_handle, "buddy-unidle",
						plugin, PURPLE_CALLBACK(on_buddy_unidle), NULL);

	purple_signal_connect(conv_handle, "received-im-msg",
						plugin, PURPLE_CALLBACK(on_im_recv), NULL);
	purple_signal_connect(conv_handle, "sent-im-msg",
						plugin, PURPLE_CALLBACK(on_im_send), NULL);

	purple_signal_connect(purple_connections_get_handle(), "signed-on",
						plugin, PURPLE_CALLBACK(on_signon), NULL);
	purple_signal_connect(purple_connections_get_handle(), "signed-off",
						plugin, PURPLE_CALLBACK(on_signoff), NULL);

	return TRUE;
}

static gboolean purple_plugin_remove(PurplePlugin *h)
{
	config = NULL;

	soundlist_free();
	messagelist_free();

	return TRUE;
}

static GtkWidget *purple_plugin_config_gtk(PurplePlugin *plugin)
{
	config = create_config();

	setup_list();
	set_option_entries();

	gtk_widget_show_all(config);

	return config;
}

static PurplePluginUiInfo ui_info = {
	purple_plugin_config_gtk,
	0 /* page_num (reserved) */
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

	SMARTEAR_PLUGIN_ID,
	N_("SmartEar"),
	SMARTEAR_VERSION,
	N_("Assign different sounds to different buddies and groups"),
	N_("Allows you to assign different "
	   "sounds to play for different buddies or whole groups of buddies.  "
	   "SmartEar allows you to opt to play sounds when a buddy sends you an "
	   "IM, signs on, returns from away or idle, or any combination of these, so "
	   "you'll know by the sound what the important people are doing.  "),
	"Matt Perry <guy@fscked.org>",
	"http://somewhere.fscked.org/smartear/",

	// old
	purple_plugin_init, // load
	purple_plugin_remove, // unload
	NULL, // destroy

	&ui_info,
	NULL,
	NULL,
	NULL
};

static void init_plugin(PurplePlugin *plugin)
{
}

PURPLE_INIT_PLUGIN(smartear, init_plugin, info);

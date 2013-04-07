/*************************************************************************
 * GTK Timezone test program
 * by Martijn van Oosterhout <kleptog@svana.org> (C) April 2006
 * Licenced under the GNU General Public Licence version 2.
 *
 * A test program to play with different ways that user could select from
 * the huge list of timezones. Eventually things tested here should migrate
 * to the module itself.
 *************************************************************************/

#include <gtk/gtk.h>
#include <string.h>
#include <ctype.h>
#include "recurse.h"

#define PACKAGE "Hello World"
#define VERSION "0.1"

#define DISABLED_STRING "<Disabled>"
#define DEFAULT_STRING "<Default>"
#define MORE_STRING "More..."
/*
 * Terminate the main loop.
 */
static void
on_destroy(GtkWidget * widget, gpointer data)
{
    gtk_main_quit();
}

enum
{
    STRING_COLUMN,
    N_COLUMNS
};

struct nodestate
{
#ifdef USE_COMBOBOX
    GtkTreeIter iter;
#else
    GtkWidget *submenu;
#endif
    gchar *string;
};

struct state
{
#ifdef USE_COMBOBOX
    GtkTreeStore *store;
    GtkTreeIter *extra;
#else
    GtkWidget *base;
    GtkWidget *extra;
#endif
    int currdepth;
    struct nodestate stack[4];
};

static inline const char *
menuitem_get_label(GtkMenuItem * menuitem)
{
    return gtk_label_get_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(menuitem))));
}

static GtkWidget *
menu_get_first_menuitem(GtkMenu * menu)
{
    GList *list = gtk_container_get_children(GTK_CONTAINER(menu));
    GtkWidget *selection = GTK_WIDGET(g_list_nth_data(list, 0));
    g_list_free(list);
    return selection;
}

int
menu_select_cb(GtkMenuItem * menuitem, GtkWidget * menu)
{
    const char *label = menuitem_get_label(menuitem);

//      printf( "menuitem = %s(%p), menu = %s(%p)\n", G_OBJECT_TYPE_NAME(menuitem), menuitem, G_OBJECT_TYPE_NAME(menu), menu );

    if(label[0] == '<')
    {
        GtkWidget *selection = menu_get_first_menuitem(GTK_MENU(menu));
        gtk_widget_hide(selection);
    }
    else
    {
        char *str = g_strdup(label);

        GtkWidget *parent;

        for (;;)
        {
            GtkMenuItem *parentitem;
            const char *label2;
            char *temp;

            parent = gtk_widget_get_parent(GTK_WIDGET(menuitem));
//                      printf( "parent = %s(%p)\n", G_OBJECT_TYPE_NAME(parent), parent);
            if(menu == parent)
                break;

            parentitem = GTK_MENU_ITEM(gtk_menu_get_attach_widget(GTK_MENU(parent)));
//                      printf( "parentitem = %s(%p)\n", G_OBJECT_TYPE_NAME(parentitem), parentitem);
            label2 = menuitem_get_label(parentitem);
            if(strcmp(label2, MORE_STRING) != 0)
            {
                temp = g_strconcat(label2, "/", str, NULL);
                g_free(str);
                str = temp;
            }

            menuitem = parentitem;
        }
        {
            GtkLabel *label;

            GtkWidget *selection = menu_get_first_menuitem(GTK_MENU(menu));
            GtkOptionMenu *optionmenu = GTK_OPTION_MENU(gtk_menu_get_attach_widget(GTK_MENU(menu)));

            label = GTK_LABEL(gtk_bin_get_child(GTK_BIN(selection)));

            gtk_label_set_text(label, str);
            gtk_widget_show(GTK_WIDGET(selection));
            gtk_option_menu_set_history(optionmenu, 0);
            g_free(str);
        }
    }
    return 0;
}

int
make_menu_cb(char *path, struct state *state)
{
    int i, j;

    char **elements;

    /* Here we ignore strings not beginning with uppercase, since they are auxilliary files, not timezones */
    if(!isupper(path[0]))
        return 0;

    elements = g_strsplit(path, "/", 4);

    for (i = 0; i < state->currdepth && state->stack[i].string; i++)
    {
        if(strcmp(elements[i], state->stack[i].string) != 0)
            break;
    }
    /* i is now the index of the first non-matching element, so free the rest */
    for (j = i; j < state->currdepth; j++)
        g_free(state->stack[j].string);
    state->currdepth = i;

    while (elements[i])
    {
#ifdef USE_COMBOBOX
        GtkTreeIter *parent = (i == 0) ? NULL : &state->stack[i - 1].iter;
#else
        GtkWidget *parent = (i == 0) ? state->base : state->stack[i - 1].submenu;
        GtkWidget *menuitem;
#endif

        if(i == 0 && elements[1] == NULL)
            parent = state->extra;

#ifdef USE_COMBOBOX
        gtk_tree_store_append(state->store, &state->stack[i].iter, parent);
        gtk_tree_store_set(state->store, &state->stack[i].iter, STRING_COLUMN, elements[i], -1);
        state->stack[i].string = g_strdup(elements[i]);
        state->currdepth++;
#else
        menuitem = gtk_menu_item_new_with_label(elements[i]);
        gtk_menu_append(parent, menuitem);

        if(elements[i + 1] != NULL)     /* Has submenu */
        {
            state->stack[i].submenu = gtk_menu_new();
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), state->stack[i].submenu);

            state->currdepth++;
            state->stack[i].string = g_strdup(elements[i]);
        }
        else
            g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(menu_select_cb),
                             state->base);

#endif

        i++;
    }
    g_strfreev(elements);
    return 0;
}

GtkWidget *
make_menu2(char *selected)
{
    int i;
    struct state state;

#ifdef USE_COMBOBOX
    GtkTreeStore *store = gtk_tree_store_new(N_COLUMNS, /* Total number of columns */
                                             G_TYPE_STRING);    /* Timezone              */
    GtkWidget *tree;

    GtkCellRenderer *renderer;
    GtkTreeIter iter1, iter2;

    gtk_tree_store_append(store, &iter1, NULL); /* Acquire an iterator */
    gtk_tree_store_set(store, &iter1, STRING_COLUMN, DISABLED_STRING, -1);
    gtk_tree_store_append(store, &iter1, NULL); /* Acquire an iterator */
    gtk_tree_store_set(store, &iter1, STRING_COLUMN, DEFAULT_STRING, -1);
    gtk_tree_store_append(store, &iter2, &iter1);
    gtk_tree_store_set(store, &iter1, STRING_COLUMN, MORE_STRING, -1);

    state.store = store;
    state.extra = &iter1;
#else

    GtkWidget *menu;
    GtkWidget *optionmenu, *menuitem, *selection;

    menu = gtk_menu_new();
    menuitem = gtk_menu_item_new_with_label(selected);
    gtk_menu_append(menu, menuitem);
    selection = menuitem;

    menuitem = gtk_menu_item_new_with_label(DISABLED_STRING);
    g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(menu_select_cb), menu);
    gtk_menu_append(menu, menuitem);
    menuitem = gtk_menu_item_new_with_label(DEFAULT_STRING);
    g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(menu_select_cb), menu);
    gtk_menu_append(menu, menuitem);
    menuitem = gtk_menu_item_new_with_label(MORE_STRING);
    gtk_menu_append(menu, menuitem);

    state.base = menu;
    state.extra = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), state.extra);
#endif
    state.currdepth = 0;

    recurse_directory("/usr/share/zoneinfo", (DirRecurseMatch) make_menu_cb, &state);

    for (i = 0; i < state.currdepth; i++)
        g_free(state.stack[i].string);

#ifdef USE_COMBOBOX
    tree = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(tree), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(tree), renderer, "text", 0, NULL);

    gtk_widget_show_all(tree);
    return tree;
#else
    optionmenu = gtk_option_menu_new();
    gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);
    gtk_widget_show_all(optionmenu);

    if(strcmp(selected, "") == 0)
    {
        gtk_option_menu_set_history(GTK_OPTION_MENU(optionmenu), 2);
        gtk_widget_hide(selection);
    }
    else if(strcmp(selected, "none") == 0)
    {
        gtk_option_menu_set_history(GTK_OPTION_MENU(optionmenu), 1);
        gtk_widget_hide(selection);
    }
    else
    {
        gtk_option_menu_set_history(GTK_OPTION_MENU(optionmenu), 0);
    }

    return optionmenu;
#endif

}

int
main(int argc, char *argv[])
{
    GtkWidget *window;
    GtkWidget *label;
    GtkWidget *menu;
    GtkWidget *frame;

    gtk_init(&argc, &argv);

    /* create the main, top level, window */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    /* give the window a 20px wide border */
    gtk_container_set_border_width(GTK_CONTAINER(window), 20);

    /* give it the title */
    gtk_window_set_title(GTK_WINDOW(window), PACKAGE " " VERSION);

    /* open it a bit wider so that both the label and title show up */
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 50);

    /* Connect the destroy event of the window with our on_destroy function
     * When the window is about to be destroyed we get a notificaiton and
     * stop the main GTK loop
     */
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(on_destroy), NULL);

    /* Create the "Hello, World" label  */
    label = gtk_label_new("Select a timezone:");
    gtk_widget_show(label);

    frame = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(frame);

    /* and insert it into the main window  */
    gtk_container_add(GTK_CONTAINER(window), frame);
    gtk_container_add(GTK_CONTAINER(frame), label);

    menu = make_menu2("none");

    gtk_container_add(GTK_CONTAINER(frame), menu);
    gtk_widget_show(menu);

    /* make sure that everything, window and label, are visible */
    gtk_widget_show(window);

    /* start the main loop */
    gtk_main();

    return 0;
}

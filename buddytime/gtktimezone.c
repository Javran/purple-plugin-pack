/*************************************************************************
 * GTK Timezone widget module
 * by Martijn van Oosterhout <kleptog@svana.org> (C) April 2006
 * Licenced under the GNU General Public Licence version 2.
 *
 * This module creates the GTK widget used to select timezones. It's here to
 * clearly seperate the GTK stuff from the plugin itself.
 *************************************************************************/

#include <gtk/gtk.h>
#include <string.h>
#include <ctype.h>
#include "recurse.h"

#define DISABLED_STRING "<Disabled>"
#define DEFAULT_STRING "<Default>"
#define MORE_STRING "More..."

struct nodestate
{
    GtkWidget *submenu;
    gchar *string;
};

struct state
{
    GtkWidget *base;
    GtkWidget *extra;
    int currdepth;
    struct nodestate stack[4];
};

static inline const char *
menuitem_get_label(GtkMenuItem * menuitem)
{
    return gtk_label_get_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(menuitem))));
}

static GtkMenuItem *
menu_get_first_menuitem(GtkWidget * menu)
{
    GList *list = gtk_container_get_children(GTK_CONTAINER(menu));
    GtkMenuItem *selection = GTK_MENU_ITEM(g_list_nth_data(list, 0));
    g_list_free(list);
    return selection;
}

static int
menu_select_cb(GtkMenuItem * menuitem, GtkWidget * menu)
{
    const char *label = menuitem_get_label(menuitem);

    if(label[0] == '<')
    {
        GtkWidget *selection = GTK_WIDGET(menu_get_first_menuitem(menu));
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
            if(menu == parent)
                break;

            parentitem = GTK_MENU_ITEM(gtk_menu_get_attach_widget(GTK_MENU(parent)));
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

            GtkMenuItem *selection = menu_get_first_menuitem(menu);
            GtkOptionMenu *optionmenu = GTK_OPTION_MENU(gtk_menu_get_attach_widget(GTK_MENU(menu)));

            label = GTK_LABEL(gtk_bin_get_child(GTK_BIN(selection)));

            gtk_label_set_text(label, str);
            gtk_widget_show(GTK_WIDGET(selection));
            gtk_option_menu_set_history(optionmenu, 0);

            printf("optionmenu=%p, menu=%p, menuitem=%p, label=%p\n", optionmenu, menu, selection,
                   label);
            g_free(str);
        }
    }
    return 0;
}

static int
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
        GtkWidget *parent = (i == 0) ? state->base : state->stack[i - 1].submenu;
        GtkWidget *menuitem;

        if(i == 0 && elements[1] == NULL)
            parent = state->extra;

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

        i++;
    }
    g_strfreev(elements);
    return 0;
}

void *
make_timezone_menu(const char *selected)
{
    int i;
    struct state state;

    GtkWidget *menu;
    GtkWidget *optionmenu, *menuitem, *selection;

    if(selected == NULL)
        selected = "";

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

    state.currdepth = 0;

    recurse_directory("/usr/share/zoneinfo", (DirRecurseMatch) make_menu_cb, &state);

    for (i = 0; i < state.currdepth; i++)
        g_free(state.stack[i].string);

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
}

const char *
get_timezone_menu_selection(void *widget)
{
    GtkOptionMenu *menu = GTK_OPTION_MENU(widget);

    int sel = gtk_option_menu_get_history(menu);
    if(sel == 2)                /* Default */
        return NULL;
    if(sel == 1)                /* Disabled */
        return "none";

    GtkLabel *l = GTK_LABEL(gtk_bin_get_child(GTK_BIN(menu)));
    return gtk_label_get_text(l);
}

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
#include <gtkprefs.h>
#include <gtkblist.h>
#include <gtkutils.h>
#include <prefs.h>
#include <version.h>

#include "../common/i18n.h"

#include "gtkticker.h"
#include "stocker_prefs.h"

#define CHANGE_INCREASE	"<span color=\"green\">+%0.02f</span>"
#define CHANGE_DECREASE	"<span color=\"red\">%0.02f</span>"
#define CHANGE_NONE		"%0.02f"

/******************************************************************************
 * structs
 *****************************************************************************/
typedef struct {
	GtkWidget *box;
	GtkWidget *change;

	gchar *symbol;
} StockerQuote;

/******************************************************************************
 * globals
 *****************************************************************************/
static GtkWidget *ticker = NULL;
static GList *quotes = NULL;

/******************************************************************************
 * other crap
 *****************************************************************************/
static gboolean
stocker_create() {
	PurpleBuddyList *blist;
	PidginBuddyList *gtkblist;

	if(GTK_IS_WIDGET(ticker))
		gtk_widget_destroy(ticker);

	blist = purple_get_blist();
	if(!blist)
		return FALSE;

	gtkblist = PIDGIN_BLIST(blist);

	ticker = gtk_ticker_new();
	gtk_box_pack_start(GTK_BOX(gtkblist->vbox), ticker, FALSE, FALSE, 0);
	gtk_widget_show(ticker);

	GtkWidget *t = gtk_label_new("test");
	gtk_ticker_add(GTK_TICKER(ticker), t);
	gtk_widget_show(t);
	return TRUE;
}

static void
stocker_blist_created(PurpleBuddyList *blist, gpointer data) {
	stocker_create();
}

/******************************************************************************
 * plugin crap
 *****************************************************************************/
static gboolean
stocker_load(PurplePlugin *plugin) {
	if(!stocker_create()) {
		purple_signal_connect(pidgin_blist_get_handle(), "gtkblist-created",
							plugin,
							PURPLE_CALLBACK(stocker_blist_created), NULL);
	}

	return TRUE;
}

static gboolean
stocker_unload(PurplePlugin *plugin) {
	if(GTK_IS_WIDGET(ticker))
		gtk_widget_destroy(ticker);
	ticker = NULL;

	return TRUE;
}

static PidginPluginUiInfo stocker_ui_info = { stocker_prefs_get_frame };

static PurplePluginInfo stocker_info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	PIDGIN_PLUGIN_TYPE,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,

	"gtk-plugin_pack-stocker",
	NULL,
	PP_VERSION,
	NULL,
	NULL,
	"Gary Kramlich <amc_grim@users.sf.net>",
	PP_WEBSITE,

	stocker_load,
	stocker_unload,
	NULL,

	&stocker_ui_info,
	NULL,
	NULL,
	NULL
};

static void
stocker_init(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	stocker_info.name = _("Stocker");
	stocker_info.summary = _("A stock ticker");
	stocker_info.description =
		_("Adds a stock ticker similar to the one in the Windows AIM client to"
			" the bottom of the buddy list.");

	stocker_prefs_init();
}

PURPLE_INIT_PLUGIN(stocker, stocker_init, stocker_info)

/*
 * Stocker - Adds a stock ticker to the buddy list
 * Copyright (C) 2005-2008 Gary Kramlich <grim@reaperworld.com>
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

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <gtkplugin.h>
#include <gtkprefs.h>
#include <gtkblist.h>
#include <gtkutils.h>
#include <prefs.h>

#include "gtkticker.h"
#include "stocker_prefs.h"

#define URL_REQUEST "http://quotewebvip-m01.blue.aol.com/?action=aim&syms=%s&fields=nspg"

#define CHANGE_INCREASE	"<span color=\"green3\">%+0.04g</span>"
#define CHANGE_DECREASE	"<span color=\"red\">%+0.04g</span>"
#define CHANGE_NONE		"%0.04g"

/******************************************************************************
 * structs
 *****************************************************************************/
#define STOCKER_QUOTE(obj)	((StockerQuote *)(obj))

typedef struct {
	gchar *symbol;

	GtkWidget *label;

	guint ref;
} StockerQuote;

/******************************************************************************
 * globals
 *****************************************************************************/
static GtkWidget *ticker = NULL;
static GHashTable *quotes = NULL;
static guint quotes_id = 0, interval_id = 0, interval_timer = 0;

/******************************************************************************
 * Quote stuff
 *****************************************************************************/
static StockerQuote *
stocker_quote_new(const gchar *symbol) {
	StockerQuote *ret = g_new0(StockerQuote, 1);
	gchar *label = NULL;

	ret->symbol = g_strdup(symbol);

	label = g_strdup_printf("%s (refreshing)", symbol);
	ret->label = gtk_label_new(label);
	g_free(label);
	
	gtk_ticker_add(GTK_TICKER(ticker), ret->label);
	gtk_widget_show(ret->label);

	ret->ref = 1;

	return ret;
}

static void
stocker_quote_ref(StockerQuote *quote) {
	quote->ref++;
}

static void
stocker_quote_unref(StockerQuote *quote) {
	quote->ref--;

	if(quote->ref != 0)
		return;

	g_free(quote->symbol);

	gtk_widget_destroy(quote->label);

	g_free(quote);

	quote = NULL;
}

static void
stocker_quote_update(StockerQuote *quote, const gchar *name, gdouble current,
					 gdouble change)
{
	GString *str = g_string_sized_new(512);

	g_string_append_printf(str,
						   "<span weight=\"bold\">%s</span> "
						   "<span size=\"small\">(%s)</span> $%g ",
						   name, quote->symbol, current);
	if(change < 0.0)
		g_string_append_printf(str, CHANGE_DECREASE, change);
	else if(change > 0.0)
		g_string_append_printf(str, CHANGE_INCREASE, change);
	else
		g_string_append_printf(str, CHANGE_NONE, change);

	gtk_label_set_markup(GTK_LABEL(quote->label), str->str);

	g_string_free(str, TRUE);
}

/******************************************************************************
 * main stuff
 *****************************************************************************/
static void
stocker_refresh_url_cb(PurpleUtilFetchUrlData *url_data, gpointer data,
					   const gchar *text, gsize len, const gchar *errmsg)
{
	const gchar *p = text;
	gchar *t;

	while((p = g_strstr_len(p, strlen(p), "DATA="))) {
		const gchar *name = NULL, *symbol = NULL;
		gdouble current = 0.0, change = 0.0;

		/* move paste the data text */
		p += 5;

		/* find the name */
		t = strchr(p, ';');
		*t = '\0';
		name = p;
		
		/* find the symbol */
		p = t + 1;
		t = strchr(p, ';');
		*t = '\0';
		symbol = p;

		/* find the current price */
		p = t + 1;
		t = strchr(p, ';');
		*t = '\0';
		current = atof(p);

		/* find the change */
		p = t + 1;
		t = strchr(p, '\r');
		*t = '\0';
		change = atof(p);

		/* now move p to the EOL */
		p = t + 1;

		if(symbol) {
			StockerQuote *quote = g_hash_table_lookup(quotes, symbol);

			if(quote) {
				stocker_quote_update(quote, name, current, change);
			}
		}
	}
}

static void
stocker_refresh_helper(gpointer k, gpointer v, gpointer d) {
	GString *str = (GString *)d;
	gchar *symbol = (gchar *)k;

	g_string_append_printf(str, "%s%s",
						   (str->len > 0) ? "," : "",
						   symbol);
}

static void
stocker_refresh(void) {
	GString *syms = g_string_sized_new(64);
	gchar *url = NULL;

	g_hash_table_foreach(quotes, stocker_refresh_helper, syms);

	url = g_strdup_printf(URL_REQUEST, syms->str);
	g_string_free(syms, TRUE);

	purple_util_fetch_url(url, TRUE, "purple", TRUE,
						  stocker_refresh_url_cb,
						  NULL);
	g_free(url);
}

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
	gtk_ticker_set_spacing(GTK_TICKER(ticker), 16);
	gtk_ticker_start_scroll(GTK_TICKER(ticker));
	gtk_widget_show_all(ticker);

	return TRUE;
}

static void
stocker_blist_created(PurpleBuddyList *blist, gpointer data) {
	stocker_create();
}

static void
stocker_quotes_refresh(GList *symbols) {
	StockerQuote *quote = NULL;
	GHashTable *new_quotes = NULL, *temp = NULL;
	GList *l = NULL;

	/* this is a bit more complicated than I'd like, but we need a way to be
	 * able to remove quotes that we don't know by name.  So we create a new
	 * hashtable, copy all the existing quotes over to it, add the new ones,
	 * then delete the old table and use the new one.
	 */

	new_quotes = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
									   (GDestroyNotify)stocker_quote_unref);

	for(l = symbols; l; l = l->next) {
		gchar *symbol = l->data;
		
		/* sanity check to make sure we have a symbol */
		if(!symbol)
			continue;

		/* look for a quote */
		quote = g_hash_table_lookup(quotes, symbol);

		if(quote) {
			/* ref the quote so it stays alive */
			stocker_quote_ref(quote);
		} else {
			/* this is a new symbol, create a quote for it */
			quote = stocker_quote_new(symbol);
		}

		/* insert the quote into the new hashtable */
		g_hash_table_insert(new_quotes, g_strdup(symbol), quote);
	}

	/* hold onto the old pointer */
	temp = quotes;

	/* update the pointer to the updated list */
	quotes = new_quotes;

	/* kill the old table */
	g_hash_table_destroy(temp);

	/* refresh everything */
	stocker_refresh();
}

static void
stocker_quotes_changed_cb(const gchar *name, PurplePrefType type,
						  gconstpointer value, gpointer data)
{
	stocker_quotes_refresh((GList *)value);
}

static gboolean
stocker_refresh_cb(gpointer data) {
	stocker_refresh();

	return TRUE;
}

static void
stocker_interval_changed_cb(const gchar *name, PurplePrefType type,
							gconstpointer value, gpointer data)
{
	gint new_time = GPOINTER_TO_INT(value);

	/* remove the old timer */
	purple_timeout_remove(interval_timer);

	/* add the new one */
	interval_timer = purple_timeout_add_seconds(new_time * 60,
												stocker_refresh_cb, NULL);
}

/******************************************************************************
 * plugin crap
 *****************************************************************************/
static gboolean
stocker_load(PurplePlugin *plugin) {
	void *prefs_handle = purple_prefs_get_handle();
	gint interval;

	stocker_create();

	quotes = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
								   (GDestroyNotify)stocker_quote_unref);

	purple_signal_connect(pidgin_blist_get_handle(), "gtkblist-created",
						  plugin,
						  PURPLE_CALLBACK(stocker_blist_created), NULL);

	quotes_id = purple_prefs_connect_callback(prefs_handle, PREF_SYMBOLS,
											  stocker_quotes_changed_cb,
											  NULL);
	interval_id = purple_prefs_connect_callback(prefs_handle, PREF_INTERVAL,
												stocker_interval_changed_cb,
												NULL);

	interval = 60 * purple_prefs_get_int(PREF_INTERVAL);
	interval_timer = purple_timeout_add_seconds(interval, stocker_refresh_cb,
												NULL);

	stocker_quotes_refresh(purple_prefs_get_string_list(PREF_SYMBOLS));

	stocker_refresh();

	return TRUE;
}

static gboolean
stocker_unload(PurplePlugin *plugin) {
	return FALSE;
}

static void
stocker_destroy(PurplePlugin *plugin) {
	purple_timeout_remove(interval_timer);

	if(GTK_IS_WIDGET(ticker))
		gtk_widget_destroy(ticker);
	ticker = NULL;
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
	"Gary Kramlich <grim@reaperworld.com>",
	PP_WEBSITE,

	stocker_load,
	stocker_unload,
	stocker_destroy,

	&stocker_ui_info,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static void
stocker_init(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
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

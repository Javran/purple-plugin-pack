/*
 * Gaim Plugin Pack
 * Copyright (C) 2003-2005
 * See ../AUTHORS for a list of all authors
 *
 * im_buzz: a plugin to implement the Yahoo! BUZZ feature.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <cmds.h>
#include <conversation.h>
#include <debug.h>
#include <gtkconv.h>
#include <gtkplugin.h>
#include <gtkprefs.h>
#include <gtkutils.h>
#include <prefs.h>
#include <version.h>

#include "../common/i18n.h"
#include "../common/gpp_compat.h"

#include <string.h>

#define YIB_STOCK_BUZZ "yib-buzz"
#define YIB_BUZZ_MSG "<ding>"
#define PRPL_YAHOO "prpl-yahoo"

static GList *buttons = NULL;
static GtkIconFactory *icon_factory;
static GaimCmdId yib_cmd;
static guint button_type_cb_id;

static gboolean
yahoo_extended_yahoo_filter(GaimAccount *account)
{
	const char *prpl_id = gaim_account_get_protocol_id(account);

	if(!prpl_id)
		return FALSE;

	if(!strcmp(prpl_id, PRPL_YAHOO))
		return TRUE;

	return FALSE;
}

static int send_buzz(GtkWidget *entry, GdkEventKey *event, gpointer data) {
	GaimConversation *conv=(GaimConversation *)data;
	GaimConnection *gc;
	GaimAccount *account;
	const char* name;
	char *buf;

	gc = gaim_conversation_get_gc(conv);
	name = gaim_conversation_get_name(conv);

	gaim_debug(GAIM_DEBUG_INFO, "buzz", "send_buzz\n");

	if (event->state & GDK_CONTROL_MASK) {
		if (event->keyval == 'g' || event->keyval == 'G') {
			/* check for right protocol */
			account=gaim_connection_get_account(gc);
			gaim_debug(GAIM_DEBUG_MISC, "buzz", gaim_account_get_protocol_id(account));
			if (strncmp(gaim_account_get_protocol_id(account), PRPL_YAHOO,strlen(PRPL_YAHOO))==0) {
				buf = g_strdup_printf("            %s", YIB_BUZZ_MSG);
				/* show it in the message window */
				gaim_conversation_write(conv, NULL, _("Buzz!!."), GAIM_MESSAGE_RECV|GAIM_MESSAGE_SYSTEM|GAIM_MESSAGE_NO_LOG , time(NULL));
				/* send it across */
				if (conv != NULL && gc != NULL && name != NULL) {
					gaim_conv_im_set_type_again(GAIM_CONV_IM(conv), TRUE);
					serv_send_im(gc, name, YIB_BUZZ_MSG,0);
					gaim_debug(GAIM_DEBUG_MISC, "buzz", "spitted <ding>...\n");
				}
				g_free(buf);
			}
		}
	}
	return 0;
}

static void
yib_button_clicked_cb(GtkButton *button, gpointer data) {

	GaimConversation *conv = g_object_get_data(G_OBJECT(button), "conv");
	GaimAccount *account = gaim_conversation_get_account(conv);
	const char *username = gaim_account_get_username(account);
	
	/* need to check what protocol conv->name is on */
	if (!yahoo_extended_yahoo_filter(gaim_conversation_get_account(conv))) {
		gaim_conv_im_write(GAIM_CONV_IM(conv), "",
			_("Please don't press this button again"), GAIM_MESSAGE_NICK|GAIM_MESSAGE_RECV, time(NULL));
		gaim_conv_im_send(GAIM_CONV_IM(conv), "I have pressed the foolish monkey button.\n");
		gaim_debug(GAIM_DEBUG_INFO, "yahoo",
		   "Sending <ding> on account %s to person %s failed! (not a yahoo buddy or account).\n", username, conv->name);
	} else {
		gaim_debug(GAIM_DEBUG_INFO, "yahoo",
		   "Sending <ding> on account %s to person %s.\n", username, conv->name);
		gaim_conv_im_send(GAIM_CONV_IM(conv), YIB_BUZZ_MSG);
		gaim_conv_im_write(GAIM_CONV_IM(conv), "", _("Buzz!!"), GAIM_MESSAGE_NICK|GAIM_MESSAGE_RECV, time(NULL));
	}
}

static GaimCmdRet
yib_cmd_cb(GaimConversation *c, const gchar *cmd, gchar **args, gchar **error, void *data) {

	GaimAccount *account = gaim_conversation_get_account(c);
	const char *username = gaim_account_get_username(account);

	if (!yahoo_extended_yahoo_filter(gaim_conversation_get_account(c))) {
		gaim_conv_im_write(GAIM_CONV_IM(c), "",
			_("Please don't press this button again"), GAIM_MESSAGE_NICK|GAIM_MESSAGE_RECV, time(NULL));
		gaim_debug(GAIM_DEBUG_INFO, "yahoo",
			   "Sending <ding> on account %s to person %s failed! (not a yahoo buddy or account).\n", username, c->name);
		return GAIM_CMD_RET_FAILED;
	} else {
		if(*args && args[0]) {
			*error = g_strdup("eek!");
			return GAIM_CMD_RET_FAILED;
		}

		gaim_debug(GAIM_DEBUG_INFO, "yahoo",
			   "Sending <ding> on account %s to person %s.\n", username, c->name);
		gaim_conv_im_send(GAIM_CONV_IM(c), YIB_BUZZ_MSG);
		gaim_conv_im_write(GAIM_CONV_IM(c), "", _("Buzz!!"), GAIM_MESSAGE_NICK|GAIM_MESSAGE_RECV, time(NULL));
		return GAIM_CMD_RET_OK;
	}
}

static void
yib_add_button(GaimConversation *conv) {
	GaimGtkConversation *gtkconv = GAIM_GTK_CONVERSATION(conv);
	GaimConversationType type = gaim_conversation_get_type(conv);
	GtkWidget *button = NULL ;
	GaimAccount *account;
	const char *prpl_id;

	if(type != GAIM_CONV_TYPE_IM || !gaim_prefs_get_bool("/plugins/gtk/bleet/yahoo-im/buzz"))
		return;

	account = gaim_conversation_get_account(conv);
	prpl_id = gaim_account_get_protocol_id(account);

	if (strcmp("prpl-yahoo", prpl_id))
 		return;

	button = gaim_gtkconv_button_new(YIB_STOCK_BUZZ, "Bell", "Yahoo-IM-Buzz",
									 gtkconv->tooltips, yib_button_clicked_cb,
									 (gpointer)conv);

	g_object_set_data(G_OBJECT(button), "conv", conv);

	buttons = g_list_append(buttons, (gpointer)button);

	gtk_box_pack_end(GTK_BOX(gtkconv->bbox), button, TRUE, TRUE, 0);
	gtk_widget_show(button);
	gtk_size_group_add_widget(gtkconv->sg, button);
}

static void
yib_show_buttons(GaimConversationType type) {
	GList *wins, *convs;
	GaimConvWindow *window;
	GaimConversation *conv;

	for(wins = gaim_get_windows(); wins; wins = wins->next) {
		window = (GaimConvWindow *)wins->data;

		for(convs = gaim_conv_window_get_conversations(window); convs;
			convs = convs->next)
		{
			conv = (GaimConversation *)convs->data;

			/*if it's an IM window, and it's a yahoo*/
			if(gaim_conversation_get_type(conv) == type && 
				yahoo_extended_yahoo_filter(gaim_conversation_get_account(conv)))
				yib_add_button(conv);
		}
	}
}

static void
yib_hide_buttons(GaimConversationType type) {
	GaimConversation *conv;
	GtkWidget *button;
	GList *l, *l_next;

	for(l = buttons; l != NULL; l = l_next) {
		l_next = l->next;

		button = GTK_WIDGET(l->data);
		conv = g_object_get_data(G_OBJECT(button), "conv");

		/*if it's an IM window, and it's a yahoo*/
		if(gaim_conversation_get_type(conv) == type && 
			yahoo_extended_yahoo_filter(gaim_conversation_get_account(conv))) {
			gtk_widget_destroy(button);

			buttons = g_list_remove(buttons, l->data);
		}
	}
}

static void
yib_im_button_show_cb(const char *name, GaimPrefType type, gpointer val,
					  gpointer data)
{
	gboolean show = GPOINTER_TO_INT(val);

	if(show)
		yib_show_buttons(GAIM_CONV_TYPE_IM);
	else
		yib_hide_buttons(GAIM_CONV_TYPE_IM);
}

static void
yib_button_type_changed_cb(const char *name, GaimPrefType type,
						   gpointer val, gpointer data)
{
	GaimConversation *conv;
	GaimGtkConversation *gtkconv;
	GtkWidget *button;
	GList *l, *tmp = NULL;

	for(l = buttons; l != NULL; l = l->next) {
		button = GTK_WIDGET(l->data);
		conv = g_object_get_data(G_OBJECT(button), "conv");

		gtkconv = GAIM_GTK_CONVERSATION(conv);

		button = gaim_gtkconv_button_new(YIB_STOCK_BUZZ, "Bell", "Yahoo-IM-Buzz",
										 gtkconv->tooltips,
										 yib_button_clicked_cb, (gpointer)conv);

		gtk_box_pack_end(GTK_BOX(gtkconv->bbox), button, TRUE, TRUE, 0);
		gtk_size_group_add_widget(gtkconv->sg, button);
		gtk_widget_show(button);
		g_object_set_data(G_OBJECT(button), "conv", conv);
		tmp = g_list_append(tmp, (gpointer)button);
	}
	g_list_free(buttons);
	buttons = tmp;
}

static void
yib_popup_cb(GtkWidget *w, GtkMenu *menu, gpointer data) {
	GtkWidget *item;

	gaim_separator(GTK_WIDGET(menu));

	item = gaim_new_item_from_stock(GTK_WIDGET(menu), "Yahoo-IM-Buzz",
									YIB_STOCK_BUZZ, NULL, NULL, 0, 0, NULL);

	gtk_widget_show(item);
}

void
yib_disconnect_popup_cb(GaimConversation *conv) {
	GaimGtkConversation *gtkconv;
	gulong handle;
  	 
	if((gtkconv = GAIM_GTK_CONVERSATION(conv)) == NULL)
		return;
  	 
	if((handle = (gulong)gaim_conversation_get_data(conv, "yib-popup-handle")) == 0)
		return;

	g_signal_handler_disconnect(G_OBJECT(gtkconv->entry), handle);
}

static void
yib_conv_created_cb(GaimConversation *conv, gpointer data) {
	GaimGtkConversation *gtkconv;
	gulong handle;

	if((gtkconv = GAIM_GTK_CONVERSATION(conv)) == NULL)
		return;

	yib_add_button(conv);
	handle = g_signal_connect(G_OBJECT(gtkconv->entry), "populate-popup",
		G_CALLBACK(yib_popup_cb), (gpointer)conv);

	gaim_conversation_set_data(conv, "yib-popup-handle", (gpointer)handle);
	
}

static void
yib_hook_popups() {
	GList *wins, *convs;
	GaimConvWindow *window;
	GaimConversation *conv;
	gulong handle;

	for(wins = gaim_get_windows(); wins; wins = wins->next) {
		window = (GaimConvWindow *)wins->data;

		for(convs = gaim_conv_window_get_conversations(window); convs;
			convs = convs->next)
		{
			conv = (GaimConversation *)convs->data;
			handle = g_signal_connect(G_OBJECT(GAIM_GTK_CONVERSATION(conv)->entry),
							 "populate-popup",
							 G_CALLBACK(yib_popup_cb), conv);
			gaim_conversation_set_data(conv, "yib-popup-handle", (gpointer)handle);

		}
	}
}

static void
yib_conv_destroyed_cb(GaimConversation *conv, gpointer data) {
	GaimConversation *stored_conv;
	GtkWidget *button;
	GList *l, *l_next;

	for(l = buttons; l != NULL; l = l_next) {
		l_next = l->next;

		button = GTK_WIDGET(l->data);
		stored_conv = (GaimConversation *)g_object_get_data(G_OBJECT(button), "conv");
		if(stored_conv == conv) {
			gtk_widget_destroy(button);
			buttons = g_list_remove(buttons, l->data);
			g_signal_handlers_disconnect_by_func(G_OBJECT(GAIM_GTK_CONVERSATION(stored_conv)->entry), send_buzz, stored_conv->window);
			break;
		}
	}
}



static GtkWidget *
yib_get_config_frame(GaimPlugin *plugin) {
	GtkWidget *vbox, *frame;

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);

	frame = gaim_gtk_make_frame(vbox, "Appearance");

	gaim_gtk_prefs_checkbox("Show button in IMs",
							"/plugins/gtk/bleet/yahoo-im/buzz",
							frame);
	gtk_widget_show_all(vbox);

	return vbox;
}

static gchar *
yib_file_name(const gchar *file_name) {
	/* I'd really like to grab from some kinda theme spot, but I dunno.... */
	return g_build_filename(DATADIR, "pixmaps", "gaim", "yib", file_name,
							NULL);
}

static void
yib_add_to_stock(const gchar *file_name, const gchar *stock_name) {
	GtkIconSet *icon_set;
	GdkPixbuf *pixbuf;
	gchar *file = yib_file_name(file_name);

	pixbuf = gdk_pixbuf_new_from_file(file, NULL);
	g_free(file);

	icon_set = gtk_icon_set_new_from_pixbuf(pixbuf);
	g_object_unref(G_OBJECT(pixbuf));

	gtk_icon_factory_add(icon_factory, stock_name, icon_set);
	gtk_icon_set_unref(icon_set);
}

static void
yib_init_stock() {
	icon_factory = gtk_icon_factory_new();
	gtk_icon_factory_add_default(icon_factory);
	
	yib_add_to_stock("stock_bell.png", YIB_STOCK_BUZZ);

}

static gboolean
yib_load(GaimPlugin *plugin) {
	void *conv_handle = gaim_conversations_get_handle();
	const gchar *help = "<pre>buzz\n"
						"Let your friends know you care.";
	/*GaimGtkConversation *gtkconv=GAIM_GTK_CONVERSATION(conv_handle);*/

	/* init our stock */
	yib_init_stock();

	/* GaimGtkConversation *gtkconv=conv_handle;
	 * gtkconv = GAIM_GTK_CONVERSATION(conv_handle);
     * register the ^G with the conversation window
	 * g_signal_connect(G_OBJECT(gtkconv->entry), "key-press-event", G_CALLBACK(send_buzz), conv_handle);*/
	
	
	/* connect to the signals we need.. */
	gaim_signal_connect(conv_handle, "conversation-created", plugin,
						GAIM_CALLBACK(yib_conv_created_cb), NULL);

	gaim_signal_connect(conv_handle, "deleting-conversation", plugin,
						GAIM_CALLBACK(yib_conv_destroyed_cb), NULL);

	button_type_cb_id = gaim_prefs_connect_callback(
							"/gaim/gtk/conversations/button_type",
							yib_button_type_changed_cb, NULL);

	gaim_prefs_connect_callback("/plugins/gtk/bleet/yahoo-im/buzz",
							yib_im_button_show_cb, NULL);

	if(gaim_prefs_get_bool("/plugins/gtk/bleet/yahoo-im/buzz"))
		yib_show_buttons(GAIM_CONV_TYPE_IM);

	/* register our command */
	yib_cmd = gaim_cmd_register("buzz", "", GAIM_CMD_P_PLUGIN,
								GAIM_CMD_FLAG_IM | GAIM_CMD_FLAG_ALLOW_WRONG_ARGS |	GAIM_CMD_FLAG_PRPL_ONLY,
								"prpl-yahoo", yib_cmd_cb, help, NULL);

	/* hijack the popup menus in the imhtmls */
	yib_hook_popups();

	return TRUE;
}

static gboolean
yib_unload(GaimPlugin *plugin) {
	gaim_prefs_disconnect_callback(button_type_cb_id);

	/* remove CTRL+G hooks */	

	/* remove our buttons */
	yib_hide_buttons(GAIM_CONV_TYPE_IM);
	g_list_free(buttons);

	/* remove our popup menu item */
	gaim_conversation_foreach(yib_disconnect_popup_cb);

	/* remove our icons */
	gtk_icon_factory_remove_default(icon_factory);

	/* remove our command */
	gaim_cmd_unregister(yib_cmd);

	return TRUE;
}

static void
init_plugin(GaimPlugin *plugin) {
	gaim_prefs_add_none("/plugins/gtk/bleet");
	gaim_prefs_add_none("/plugins/gtk/bleet/yahoo-im");
	gaim_prefs_add_bool("/plugins/gtk/bleet/yahoo-im/buzz", TRUE);
}

static GaimGtkPluginUiInfo ui_info = { yib_get_config_frame };

static GaimPluginInfo yib_info = {
	GAIM_PLUGIN_MAGIC,
	GAIM_MAJOR_VERSION,
	GAIM_MINOR_VERSION,
	GAIM_PLUGIN_STANDARD,								/* type */
	GAIM_GTK_PLUGIN_TYPE,								/* ui requirement */
	0,													/* flags */
	NULL,												/* dependencies */
	GAIM_PRIORITY_DEFAULT,								/* priority */

	"gtk-plugin_pack-buzzer",							/* id */
	"Yahoo-IM-Buzz",									/* name */
	GPP_VERSION,										/* version */
	"Yahoo IM Buzz",									/* summary */
	"A small plugin at adds a button to the\n"
	"gaim Yahoo IM window, so that you\n"
	"can let your friends know you care.",				/* description */

	"Peter Lawler <bleeter from users.sf.net>",			/* author */
	GPP_WEBSITE,										/* homepage */

	yib_load,											/* load */
	yib_unload,											/* unload */
	NULL,												/* destroy */

	&ui_info,											/* ui info */
	NULL,												/* extra info */
	NULL												/* actions info */
};

GAIM_INIT_PLUGIN(Yahoo-IM-Buzz, init_plugin, yib_info)

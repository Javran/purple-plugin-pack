/*
 * Purple-XChat - Use XChat-like chats
 * Copyright (C) 2005-2008
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

#define PLUGIN_ID			"gtk-plugin_pack-xchat-chats"
#define PLUGIN_NAME			"xchat-chats"
#define PLUGIN_AUTHOR		"Sadrul Habib Chowdhury <sadrul@users.sourceforge.net>"

#define	PREFS_PREFIX		"/plugins/gtk/" PLUGIN_ID
#define	PREFS_DATE_FORMAT	PREFS_PREFIX "/date_format"

/* System headers */
#include <string.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

/* Purple headers */
#include <conversation.h>
#include <util.h>

/* Pidgin headers */
#include <gtkconv.h>
#include <gtkplugin.h>

#include "xtext.h"

static PurpleConversationUiOps *uiops = NULL;

static void (*default_write_conv)(PurpleConversation *conv, const char *name, const char *alias,
						   const char *message, PurpleMessageFlags flags, time_t mtime);
static void (*default_create_conversation)(PurpleConversation *conv);

static void (*default_destroy_conversation)(PurpleConversation *conv);

static GtkWidget* hack_and_get_widget(PidginConversation *gtkconv);
void palette_alloc (GtkWidget * widget);

typedef struct _PurpleXChat PurpleXChat;

struct _PurpleXChat
{
	GtkWidget *imhtml;
	GtkWidget *xtext;
};

static GHashTable *xchats = NULL;		/* Hashtable of xchats */

typedef enum
{
	GX_SEND,
	GX_RECV,
	GX_SYSTEM,
	GX_HIGHLIGHT,
	GX_ERROR
}PurpleXChatMessage;

static GdkColor colors[][2] = {
    /* colors for xtext */
	{ {0, 0x4c4c, 0x4c4c, 0x4c4c}, {0, 0x4c4c, 0x4c4c, 0x4c4c} },	/* Message sent */
	{ {0, 0x35c2, 0x35c2, 0xb332}, {0, 0, 0, 0} },					/* Message received */
	{ {0, 0xd9d9, 0xa6a6, 0x4141}, {0, 0xd9d9, 0xa6a6, 0x4141} }, 	/* System message */
	{ {0, 0xc7c7, 0x3232, 0x3232}, {0, 0xc7c7, 0x3232, 0x3232} },		/* Highlight message */
	{ {0, 0xc7c7, 0x3232, 0x3232}, {0, 0x4c4c, 0x4c4c, 0x4c4c} },	/* Error message */
};
#if 0
    {0, 0xcccc, 0xcccc, 0xcccc}, /* 16 white */
    {0, 0x0000, 0x0000, 0x0000}, /* 17 black */
    {0, 0x35c2, 0x35c2, 0xb332}, /* 18 blue */
    {0, 0x2a3d, 0x8ccc, 0x2a3d}, /* 19 green */
    {0, 0xc7c7, 0x3232, 0x3232}, /* 21 light red */
    {0, 0x8000, 0x2666, 0x7fff}, /* 22 purple */
    {0, 0xd999, 0xa6d3, 0x4147}, /* 24 yellow */
    {0, 0x3d70, 0xcccc, 0x3d70}, /* 25 green */
    {0, 0x199a, 0x5555, 0x5555}, /* 26 aqua */
    {0, 0x2eef, 0x8ccc, 0x74df}, /* 27 light aqua */
    {0, 0x451e, 0x451e, 0xe666}, /* 28 blue */
    {0, 0x4c4c, 0x4c4c, 0x4c4c}, /* 30 grey */
    {0, 0x9595, 0x9595, 0x9595}, /* 31 light grey */

    {0, 0xcccc, 0xcccc, 0xcccc}, /* 16 white */
    {0, 0x0000, 0x0000, 0x0000}, /* 17 black */
    {0, 0x35c2, 0x35c2, 0xb332}, /* 18 blue */
    {0, 0x2a3d, 0x8ccc, 0x2a3d}, /* 19 green */
    {0, 0xc7c7, 0x3232, 0x3232}, /* 21 light red */
    {0, 0x8000, 0x2666, 0x7fff}, /* 22 purple */
    {0, 0xd999, 0xa6d3, 0x4147}, /* 24 yellow */
    {0, 0x3d70, 0xcccc, 0x3d70}, /* 25 green */
    {0, 0x199a, 0x5555, 0x5555}, /* 26 aqua */
    {0, 0x2eef, 0x8ccc, 0x74df}, /* 27 light aqua */
    {0, 0x451e, 0x451e, 0xe666}, /* 28 blue */
    {0, 0x4c4c, 0x4c4c, 0x4c4c}, /* 30 grey */
    {0, 0x9595, 0x9595, 0x9595}, /* 31 light grey */

    {0, 0xffff, 0xffff, 0xffff}, /* 32 marktext Fore (white) */
    {0, 0x3535, 0x6e6e, 0xc1c1}, /* 33 marktext Back (blue) */
    {0, 0x0000, 0x0000, 0x0000}, /* 34 foreground (black) */
    {0, 0xf0f0, 0xf0f0, 0xf0f0}, /* 35 background (white) */
    {0, 0xcccc, 0x1010, 0x1010}, /* 36 marker line (red) */

    /* colors for GUI */
    {0, 0x9999, 0x0000, 0x0000}, /* 37 tab New Data (dark red) */
    {0, 0x0000, 0x0000, 0xffff}, /* 38 tab Nick Mentioned (blue) */
    {0, 0xffff, 0x0000, 0x0000}, /* 39 tab New Message (red) */
    {0, 0x9595, 0x9595, 0x9595}, /* 40 away user (grey) */
};
#endif

#if 0
/* check if a word is clickable */

static int
mg_word_check (GtkWidget * xtext, char *word, int len)
{
    session *sess = current_sess;
    int ret;

    ret = url_check_word (word, len);   /* common/url.c */
    if (ret == 0)
    {
        if (( (word[0]=='@' || word[0]=='+') && userlist_find (sess, word+1)) || userlist_find (sess, word))
            return WORD_NICK;

        if (sess->type == SESS_DIALOG)
            return WORD_DIALOG;
    }

    return ret;
}
#endif

static gboolean
is_2_4_0_or_above(void)
{
	return (purple_version_check(2, 4, 0) == NULL);
}


static GtkWidget *get_xtext(PurpleConversation *conv)
{
	PurpleXChat *gx;

	if ((gx = g_hash_table_lookup(xchats, conv)) == NULL)
	{
		PidginConversation *gtkconv;
		GtkWidget *xtext;
		GtkWidget *imhtml = NULL;
		GtkStyle *style;

		gtkconv = PIDGIN_CONVERSATION(conv);
		if (!gtkconv)
			return NULL;
		imhtml = gtkconv->imhtml;
		style = gtk_widget_get_style(imhtml);

		palette_alloc(pidgin_conv_get_window(gtkconv)->window);

		gx = g_new0(PurpleXChat, 1);

		xtext = gtk_xtext_new(colors, TRUE);

		/* TODO: Figure out a way to set the colors correctly */
		gtk_xtext_set_foreground_color(GTK_XTEXT(xtext), &style->text[0]);
		gtk_xtext_set_background_color(GTK_XTEXT(xtext), &style->base[0]);
		gtk_xtext_set_indent(GTK_XTEXT(xtext), TRUE);
		gtk_xtext_set_max_indent(GTK_XTEXT(xtext), 200);

		gx->xtext = xtext;
		gx->imhtml = hack_and_get_widget(gtkconv);

		if (!gtk_xtext_set_font(GTK_XTEXT(xtext),
					pango_font_description_to_string(style->font_desc)))
			return NULL;

		g_hash_table_insert(xchats, conv, gx);
	}
	return gx->xtext;
}

void
palette_alloc (GtkWidget * widget)
{
    int i;
    static int done_alloc = FALSE;
    GdkColormap *cmap;

    if (!done_alloc)          /* don't do it again */
    {
        done_alloc = TRUE;
        cmap = gtk_widget_get_colormap (widget);
        for (i = G_N_ELEMENTS(colors)-1; i >= 0; i--)
		{
            gdk_colormap_alloc_color (cmap, &colors[i][0], FALSE, TRUE);
            gdk_colormap_alloc_color (cmap, &colors[i][1], FALSE, TRUE);
		}
    }
}

static void purple_xchat_write_conv(PurpleConversation *conv, const char *name, const char *alias,
						   const char *message, PurpleMessageFlags flags, time_t mtime)
{
	PurpleConversationType type;
	GtkWidget *xtext;
	char *msg;
	PurpleXChatMessage col = 0;

	/* Do the usual stuff first. */
	default_write_conv(conv, name, alias, message, flags, mtime);
	
	type = purple_conversation_get_type(conv);
	if (type != PURPLE_CONV_TYPE_CHAT)
	{
		/* If it's IM, we have nothing to do. */
		return;
	}

	/* So it's a chat. Let's play. */

	xtext = get_xtext(conv);
	if (name == NULL || !strcmp(name, purple_conversation_get_name(conv)))
		name = "*";
	msg = purple_markup_strip_html(message);
	if (msg && msg[0] == '/' && msg[1] == 'm' && msg[2] == 'e' && msg[3] == ' ')
	{
		char *tmp = msg;
		msg = g_strdup_printf("%s%s", name, tmp+3);
		g_free(tmp);
		name = "*";
	}

	if (flags & PURPLE_MESSAGE_SEND)
		col = GX_SEND;
	else if (flags & PURPLE_MESSAGE_RECV)
	{
		if (flags & PURPLE_MESSAGE_NICK)
			col = GX_HIGHLIGHT;
		else
			col = GX_RECV;
	}
	else if (flags & PURPLE_MESSAGE_ERROR)
		col = GX_ERROR;
	else if ((flags & PURPLE_MESSAGE_SYSTEM) || (flags & PURPLE_MESSAGE_NO_LOG))
		col = GX_SYSTEM;

	gtk_xtext_append_indent(GTK_XTEXT(xtext)->buffer, (guchar*)name, strlen(name), colors[col][0].pixel,
								(guchar*)msg, strlen(msg), colors[col][1].pixel);
	g_free(msg);
}

#define	DEBUG_INFO(x)	\
	name = G_OBJECT_TYPE_NAME(x);	\
	printf("%s\n", name)

static GtkWidget*
hack_and_get_widget(PidginConversation *gtkconv)
{
	GtkWidget *tab_cont, *vbox, *hpaned, *frame;
	GList *list;
	const char *name;

	/* If you think this is ugly, you are right. */
	tab_cont = gtkconv->tab_cont;
	DEBUG_INFO(tab_cont);

	list = gtk_container_get_children(GTK_CONTAINER(tab_cont));
	if (!is_2_4_0_or_above()) {
		GtkWidget *pane = list->data;
		DEBUG_INFO(pane);

		vbox = GTK_PANED(pane)->child1;
	} else {
		vbox = list->data;
	}
	g_list_free(list);

	DEBUG_INFO(vbox);
	list = GTK_BOX(vbox)->children;
	while (list) {
		if (GTK_IS_PANED(((GtkBoxChild*)list->data)->widget))
			break;
		list = list->next;
	}
	hpaned = ((GtkBoxChild*)list->data)->widget;
	DEBUG_INFO(hpaned);

	frame = GTK_PANED(hpaned)->child1;

	return frame;
}

static void
purple_conversation_use_xtext(PurpleConversation *conv)
{
	PidginConversation *gtkconv;
	GtkWidget *parent, *box, *wid, *frame, *xtext;

	if (purple_conversation_get_type(conv) != PURPLE_CONV_TYPE_CHAT)
		return;

	gtkconv = PIDGIN_CONVERSATION(conv);
	if (!gtkconv)
		return;

	frame = hack_and_get_widget(gtkconv);
	parent = frame->parent;
	gtk_widget_hide_all(frame);
	g_object_ref(frame);

	box = gtk_hbox_new(FALSE, 0);
	xtext = get_xtext(conv);

	GTK_PANED(parent)->child1 = NULL;
	gtk_paned_pack1(GTK_PANED(parent), box, TRUE, TRUE);

	gtk_box_pack_start(GTK_BOX(box), xtext, TRUE, TRUE, 0);

	wid = gtk_vscrollbar_new(GTK_XTEXT(xtext)->adj);
	gtk_box_pack_start(GTK_BOX(box), wid, FALSE, FALSE, 0);
	GTK_WIDGET_UNSET_FLAGS(wid, GTK_CAN_FOCUS);

	gtk_widget_show_all(box);
	gtk_widget_realize(xtext);
}

static void
purple_xchat_create_conv(PurpleConversation *conv)
{
	default_create_conversation(conv);
	purple_conversation_use_xtext(conv);
}

static void
purple_xchat_destroy_conv(PurpleConversation *conv)
{
	PurpleXChat *gx;

	default_destroy_conversation(conv);

	gx = g_hash_table_lookup(xchats, conv);
	if (gx)
	{
		g_free(gx);
		g_hash_table_remove(xchats, conv);
	}
}

static void
workaround_for_hidden_convs(PidginConversation *gtkconv)
{
	PurpleConversation *conv = gtkconv->active_conv;

	if (purple_conversation_get_type(conv) != PURPLE_CONV_TYPE_CHAT ||
			g_hash_table_lookup(xchats, conv))
		return;
	purple_conversation_use_xtext(conv);
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	GList *list;

	uiops = pidgin_conversations_get_conv_ui_ops();

	if (uiops == NULL)
		return FALSE;

	/* Use the oh-so-useful uiops. Signals? bleh. */
	default_write_conv = uiops->write_conv;
	uiops->write_conv = purple_xchat_write_conv;

	default_create_conversation = uiops->create_conversation;
	uiops->create_conversation = purple_xchat_create_conv;

	default_destroy_conversation = uiops->destroy_conversation;
	uiops->destroy_conversation = purple_xchat_destroy_conv;

	xchats = g_hash_table_new(g_direct_hash, g_direct_equal);

	list = purple_get_chats();
	while (list)
	{
		/* TODO: We can get the message history of the conversation and populate
		 * the next xtext with that data.
		 * Note: purple_conversation_get_message_history
		 */
		purple_conversation_use_xtext(list->data);
		list = list->next;
	}

	purple_signal_connect(pidgin_conversations_get_handle(), "conversation-displayed",
			plugin, G_CALLBACK(workaround_for_hidden_convs), NULL);

	return TRUE;
}

static void remove_xtext(PurpleConversation *conv, PurpleXChat *gx, gpointer null)
{
	GtkWidget *frame, *parent;

	frame = gx->xtext->parent;
	parent = frame->parent;

	GTK_PANED(parent)->child1 = NULL;
	gx->imhtml->parent = NULL;
	gtk_paned_add1(GTK_PANED(parent), gx->imhtml);
	g_object_unref(gx->imhtml);

	gtk_widget_show_all(gx->imhtml);

	gtk_widget_destroy(frame);
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	/* Restore the default ui-ops */
	uiops->write_conv = default_write_conv;
	uiops->create_conversation = default_create_conversation;
	uiops->destroy_conversation = default_destroy_conversation;

	/* Clear up everything */
	g_hash_table_foreach(xchats, (GHFunc)remove_xtext, NULL);
	g_hash_table_destroy(xchats);

	return TRUE;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,		/* Magic				*/
	PURPLE_MAJOR_VERSION,		/* Purple Major Version	*/
	PURPLE_MINOR_VERSION,		/* Purple Minor Version	*/
	PURPLE_PLUGIN_STANDARD,		/* plugin type			*/
	PIDGIN_PLUGIN_TYPE,			/* ui requirement		*/
	0,							/* flags				*/
	NULL,						/* dependencies			*/
	PURPLE_PRIORITY_DEFAULT,	/* priority				*/

	PLUGIN_ID,					/* plugin id			*/
	NULL,						/* name					*/
	PP_VERSION,					/* version				*/
	NULL,						/* summary				*/
	NULL,						/* description			*/
	PLUGIN_AUTHOR,				/* author				*/
	PP_WEBSITE,					/* website				*/

	plugin_load,				/* load					*/
	plugin_unload,				/* unload				*/
	NULL,						/* destroy				*/

	NULL,						/* ui_info				*/
	NULL,						/* extra_info			*/
	NULL,						/* prefs_info			*/
	NULL,						/* actions				*/
	NULL,						/* reserved 1			*/
	NULL,						/* reserved 2			*/
	NULL,						/* reserved 3			*/
	NULL						/* reserved 4			*/
};

static void
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("XChat Chats");
	info.summary = _("XChat-like chats with Pidgin");
	info.description = _("You can chat in Pidgin using XChat's indented view.");

	purple_prefs_add_none(PREFS_PREFIX);
	purple_prefs_add_string(PREFS_DATE_FORMAT, "[%H:%M]");
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info)

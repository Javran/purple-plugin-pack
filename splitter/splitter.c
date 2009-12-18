/* Message Splitter Plugin v0.95
 *
 * Splits a large message into smaller messages and sends them away
 * in its place.
 *
 * Copyright (C) 2005-2007, Ike Gingerich <ike_@users.sourceforge.net>
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

#include "../common/pp_internal.h"

#include <pango/pango.h>

#ifdef _WIN32
# include <pango/pangowin32.h>
#endif

#ifndef _WIN32
# include <pango/pangocairo.h>
#endif

#include <string.h>
#include <errno.h>

#include <debug.h>
#include <notify.h>
#include <version.h>
#include <util.h>

#define PLUGIN_ID "core-ike-splitter"

/* grr */
#ifndef ENOTCONN
#define ENOTCONN 107
#endif

/* plugin constants and structures */
static const gint MIN_SPLIT_SIZE     =      32;
static const gint DEFAULT_SPLIT_SIZE =     786;
static const gint MAX_SPLIT_SIZE     =    8192;

static const gint MIN_DELAY_MS       =       0;
static const gint DEFAULT_DELAY_MS   =     500;
static const gint MAX_DELAY_MS       = 3600000; 

typedef struct
{
	char *sender_username;
	char *sender_protocol_id;
	GQueue *messages;

	PurpleConversationType type;
	union {
		char *receiver; /* IM username */
		gint id;        /* chat ID */
	};
} message_to_conv;

typedef struct {
	gint start;
	gint end;
} message_slice;

/* plugin preference variables */
static gint current_split_size;

/* initialize preferences dialog */
static PurplePluginPrefFrame *
get_plugin_pref_frame(PurplePlugin *plugin) {
	PurplePluginPrefFrame *frame;
	PurplePluginPref      *ppref;

	frame = purple_plugin_pref_frame_new();
	g_return_val_if_fail(frame != NULL, NULL);

	ppref = purple_plugin_pref_new_with_label("Message split size");
	g_return_val_if_fail(ppref != NULL, NULL);
	purple_plugin_pref_frame_add(frame, ppref);
	ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/splitter/split_size", 
							 "Letter count: ");
	g_return_val_if_fail(ppref != NULL, NULL);
	purple_plugin_pref_set_bounds(ppref, MIN_SPLIT_SIZE, MAX_SPLIT_SIZE);
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_label("Delay between messages");
	g_return_val_if_fail(ppref != NULL, NULL);
	purple_plugin_pref_frame_add(frame, ppref);
	ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/splitter/delay_ms", 
							 "ms: ");
	g_return_val_if_fail(ppref != NULL, NULL);
	purple_plugin_pref_set_bounds(ppref, MIN_DELAY_MS, MAX_DELAY_MS);
	purple_plugin_pref_frame_add(frame, ppref);

	return frame;
}

/**
 * A function to send a chat or im message to the specific conversation
 * without emitting "sending-im" or "sending-chat" signal, which would
 * cause an infinite loop for this plugin.
 *
 * taken from conversation.c with signal emission removed.
 */
static void
splitter_common_send(PurpleConversation *conv, const char *message,
                     PurpleMessageFlags msgflags)
{
	PurpleConversationType type;
	PurpleAccount *account;
	PurpleConnection *gc;
	char *displayed = NULL, *sent = NULL;

	if (strlen(message) == 0)
		return;

	account = purple_conversation_get_account(conv);
	gc = purple_conversation_get_gc(conv);

	g_return_if_fail(account != NULL);
	g_return_if_fail(gc != NULL);

	type = purple_conversation_get_type(conv);

	/* Always linkfy the text for display */
	displayed = purple_markup_linkify(message);

	if ((conv->features & PURPLE_CONNECTION_HTML) &&
		!(msgflags & PURPLE_MESSAGE_RAW))
	{
		sent = g_strdup(displayed);
	}
	else
		sent = g_strdup(message);

	msgflags |= PURPLE_MESSAGE_SEND;

	if (type == PURPLE_CONV_TYPE_IM) {
		if (sent != NULL && sent[0] != '\0')
			purple_conv_im_send_with_flags(PURPLE_CONV_IM(conv), sent, msgflags);
	}
	else {
		if (sent != NULL && sent[0] != '\0')
			purple_conv_chat_send_with_flags(PURPLE_CONV_CHAT(conv), sent, msgflags);
	}

	g_free(displayed);
	g_free(sent);
}

/* a timer based callback function that sends the next message in the queue */
static gboolean
send_message_timer_cb( message_to_conv *msg_to_conv ) {
	PurpleAccount *account;
	PurpleConversation *conv;
	gchar *msg;

	g_return_val_if_fail(msg_to_conv                     != NULL, FALSE);
	g_return_val_if_fail(msg_to_conv->messages           != NULL, FALSE);
	g_return_val_if_fail(msg_to_conv->sender_username    != NULL, FALSE);
	g_return_val_if_fail(msg_to_conv->sender_protocol_id != NULL, FALSE);

	msg = g_queue_pop_head(msg_to_conv->messages);

	if( msg == NULL )
	{
		/* clean up and terminate timer callback */
		g_queue_free(msg_to_conv->messages);
		g_free(msg_to_conv->sender_username);
		g_free(msg_to_conv->sender_protocol_id);
		if( msg_to_conv->type == PURPLE_CONV_TYPE_IM &&
			msg_to_conv->receiver != NULL )
		g_free(msg_to_conv->receiver);

		g_free(msg_to_conv);

		return FALSE;
	}
	else
	{
		/* find account info (it may have changed) and try and create a new
		   conversation window (it may have been closed) or find the existing
		   chat, and finally send the message */
		account = purple_accounts_find(msg_to_conv->sender_username,
					     msg_to_conv->sender_protocol_id);
		g_return_val_if_fail(account != NULL, FALSE);

		if( msg_to_conv->type == PURPLE_CONV_TYPE_IM && msg_to_conv->receiver != NULL )
			conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, account, msg_to_conv->receiver);
		else if( msg_to_conv->type == PURPLE_CONV_TYPE_CHAT )
			conv = purple_find_chat(account->gc, msg_to_conv->id);
		else
			conv = NULL;

		g_return_val_if_fail(conv != NULL, FALSE);

		splitter_common_send(conv, msg, PURPLE_MESSAGE_SEND);
		g_free(msg);

		return TRUE;
	}
}

/* Create/get a pango context
 *
 * On windows we use the win32 context creator, on everything else we
 * use the cairo one.
 */
PangoContext *
splitter_create_pango_context(void) {
#ifdef _WIN32
	return pango_win32_get_context();
#else
	PangoContext *context = NULL;
	PangoFontMap *fontmap = pango_cairo_font_map_get_default();

	context =
		pango_cairo_font_map_create_context(PANGO_CAIRO_FONT_MAP(fontmap));

	g_object_unref(G_OBJECT(fontmap));

	return context;
#endif
}

/* finds the first line-breakable character backwards starting from a[last]  */
static gint
find_last_break(PangoLogAttr *a, int last) {
	for(; last > 0; last-- )
		if( a[last].is_line_break == 1)
			break;

	return ( a[last].is_line_break == 1) ? last-1 : -1;
}

/* uses Pango to find all possible line break locations in a message and returns
   a PangoLogAttr array which maps to each byte of the message of length
   one larger than the message.  This must be g_free()'d */
static PangoLogAttr *
find_all_breaks(const char *message) {
	PangoContext *context;
	PangoLogAttr *a;
	GList *list;
	gint len, n_attr;

	g_return_val_if_fail(message != NULL, NULL);

	len  = strlen(message);
	n_attr = len+1;
	a = g_new0(PangoLogAttr, n_attr);

	/* init Pango */
	context = splitter_create_pango_context();

	g_return_val_if_fail(context != NULL, NULL);

	list = pango_itemize(context, message, 0, len, NULL, NULL);

	if (list != NULL && list->data != NULL)
		pango_break(message, -1, &((PangoItem*)(list->data))->analysis, a, n_attr);

	return a;
}

/* return a queue of message slices from a plain text message based on current_split_size using
   Pango to determine possible line break locations */
static GQueue *
get_message_slices(const char *message) {
	gint current_break_start, last_break_start, break_pos, len;
	message_slice *slice;
	PangoLogAttr *a;
	GQueue *q;

	q = g_queue_new();
	len = strlen(message);
	a = find_all_breaks(message);
	g_return_val_if_fail(a != NULL, NULL);

	last_break_start = current_break_start = 0;

	while(current_break_start + current_split_size < len)
	{
		break_pos = find_last_break(a + last_break_start, current_split_size);

		if( break_pos > -1 )   current_break_start += break_pos;
		else                   current_break_start += current_split_size;

		slice = g_new0( message_slice, 1 );
		slice->start = MAX( last_break_start, 0 );
		slice->end   = MIN( current_break_start, len );

		if( slice->end > slice->start )
			g_queue_push_tail(q, slice);
		else	g_free(slice);

		if( break_pos > -1 )
			current_break_start++;

		last_break_start = current_break_start;
	}

	slice = g_new0( message_slice, 1 );
	slice->start = last_break_start;
	slice->end   = len;
	g_queue_push_tail(q, slice);

	/* cleanup */
	g_free(a);

	return q;
	
}

/* takes a message, splits it up based on whitespace (ignoring HTML formatting),
   requests HTMLized slices of the splits, and returns a queue of them.  The
   messages and the queue must be freed */
static GQueue *
create_message_queue(const char *message) {
	GQueue *slices, *messages;
	message_slice *slice;
	char *stripped_message, *msg;
	gint stripped_len;

	stripped_message = purple_markup_strip_html(message);
	stripped_len = strlen(stripped_message);

	messages = g_queue_new();
	slices   = get_message_slices(stripped_message);
	g_return_val_if_fail(slices != NULL, NULL);

	while( (slice = g_queue_pop_head(slices)) != NULL )
	{
		msg = purple_markup_slice(message, slice->start, slice->end);

		if( msg != NULL )
			g_queue_push_tail(messages, msg);

		g_free(slice);
	}

	g_queue_free(slices);

	/* cleanup */
	g_free(stripped_message);

	return messages;
}

/* create message queue and prepare timer callbacks */
static void
split_and_send(message_to_conv *msg_to_conv, const char **message) {
	gint message_delay_ms;

	g_return_if_fail( msg_to_conv != NULL );
	g_return_if_fail( message     != NULL );
	g_return_if_fail( *message    != NULL );

	/* read and validate preferences */
	current_split_size = purple_prefs_get_int("/plugins/core/splitter/split_size");
	if( current_split_size > MAX_SPLIT_SIZE ) current_split_size = MAX_SPLIT_SIZE;
	if( current_split_size < MIN_SPLIT_SIZE ) current_split_size = MIN_SPLIT_SIZE;

	message_delay_ms = purple_prefs_get_int("/plugins/core/splitter/delay_ms");
	if( message_delay_ms > MAX_DELAY_MS ) message_delay_ms = MAX_DELAY_MS;
	if( message_delay_ms < MIN_DELAY_MS ) message_delay_ms = MIN_DELAY_MS;

	/* prepare message queue */
	msg_to_conv->messages = create_message_queue(*message);
	g_return_if_fail( msg_to_conv->messages != NULL );

	/* initialize message send timer */
	purple_timeout_add( (g_queue_get_length(msg_to_conv->messages) > 1) ? message_delay_ms : 0,
			  (GSourceFunc)send_message_timer_cb,
			  msg_to_conv);

	/* free the original message and ensure it does not get sent */
	g_free((char*)*message);
	*message = NULL;
}

/* initialize a chat message to potentially be split */
static void
sending_chat_msg_cb(PurpleAccount *account, const char **message, int id) {
	message_to_conv *msg_to_conv;

	purple_debug(PURPLE_DEBUG_MISC, "purple-splitter", "splitter plugin invoked\n");

	g_return_if_fail(account  != NULL);
	g_return_if_fail(message  != NULL);
	g_return_if_fail(*message != NULL);

	msg_to_conv = g_new0(message_to_conv, 1);
	g_return_if_fail( msg_to_conv != NULL );

	msg_to_conv->sender_username     = g_strdup(account->username);
	msg_to_conv->sender_protocol_id  = g_strdup(account->protocol_id);
	msg_to_conv->id                  = id;
	msg_to_conv->type                = PURPLE_CONV_TYPE_CHAT;

	split_and_send(msg_to_conv, message);
}

/* initialize an IM message to potentially be split */
static void
sending_im_msg_cb(PurpleAccount *account, const char *receiver,
                  const char **message)
{
	message_to_conv *msg_to_conv;

	purple_debug(PURPLE_DEBUG_MISC, "purple-splitter", "splitter plugin invoked\n");

	g_return_if_fail(account  != NULL);
	g_return_if_fail(receiver != NULL);
	g_return_if_fail(message  != NULL);
	g_return_if_fail(*message != NULL);

	msg_to_conv = g_new0(message_to_conv, 1);
	g_return_if_fail( msg_to_conv != NULL );

	msg_to_conv->sender_username     = g_strdup(account->username);
	msg_to_conv->sender_protocol_id  = g_strdup(account->protocol_id);
	msg_to_conv->receiver            = g_strdup(receiver);
	msg_to_conv->type                = PURPLE_CONV_TYPE_IM;

	split_and_send(msg_to_conv, message);
}

/* register "sending" message signal callback */
static gboolean
plugin_load(PurplePlugin *plugin) {
	purple_signal_connect_priority(purple_conversations_get_handle(),
			    "sending-im-msg",
			    plugin,
			    PURPLE_CALLBACK(sending_im_msg_cb),
			    NULL,
				PURPLE_SIGNAL_PRIORITY_LOWEST);
	purple_signal_connect_priority(purple_conversations_get_handle(),
			    "sending-chat-msg",
			    plugin,
			    PURPLE_CALLBACK(sending_chat_msg_cb),
			    NULL,
				PURPLE_SIGNAL_PRIORITY_LOWEST);

	return TRUE;
}


static gboolean
plugin_unload(PurplePlugin *plugin) {
	return TRUE;
}

static PurplePluginUiInfo prefs_info = {
	get_plugin_pref_frame, 0, NULL, NULL, NULL, NULL, NULL
};

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,

	PLUGIN_ID,
	NULL,
	PP_VERSION,

	NULL,
	NULL,
	"Ike Gingerich <ike_@users.sourceforge.net>",
	PP_WEBSITE,

	plugin_load,
	plugin_unload,
	NULL,

	NULL,
	NULL,
	&prefs_info,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

/* store initial preference values */
static void
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Message Splitter");
	info.summary = _("Splits a large outgoing message into smaller messages of "
	                 "a specified size.");
	info.description = info.summary;

	purple_prefs_add_none("/plugins/core/splitter");
	purple_prefs_add_int ("/plugins/core/splitter/split_size", DEFAULT_SPLIT_SIZE);
	purple_prefs_add_int ("/plugins/core/splitter/delay_ms",   DEFAULT_DELAY_MS);
}

PURPLE_INIT_PLUGIN(splitter, init_plugin, info)

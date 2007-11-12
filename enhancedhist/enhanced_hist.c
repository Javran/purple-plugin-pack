/*
 * enhanced_hist.c - Enhanced History Plugin for libpurple
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
 */

/* SUMMARY: Puts log of past conversations in the user's IM window and
 * allows the user to select the number of conversations to display from
 * purple plugin preferences.
 *
 * Author: 
 *         Andrew Pangborn  - gaim@andrewpangborn.com
 *
 * Contributors:
 *         Ankit Singla
 */


#include "conversation.h"
#include "debug.h"
#include "log.h"
#include "prefs.h"
#include "signals.h"
#include "util.h"
#include "version.h"
#include "pluginpref.h"
#include "gtkconv.h"
#include "gtkimhtml.h"
#include "gtkplugin.h"
#include "debug.h"
#include "time.h"

#define HISTORY_PLUGIN_ID "gtk-enhanced-history"

#define NUM_OF_CHATS (purple_prefs_get_int("/plugins/core/enhanced_history/int"))
#define NUM_MINS (purple_prefs_get_int("/plugins/core/enhanced_history/mins"))
#define NUM_HOURS (purple_prefs_get_int("/plugins/core/enhanced_history/hours"))
#define NUM_DAYS (purple_prefs_get_int("/plugins/core/enhanced_history/days"))
#define	PREF_DATE (purple_prefs_get_string("/plugins/core/enhanced_history/string_date"))
#define	PREF_IM (purple_prefs_get_string("/plugins/core/enhanced_history/string_im"))
#define PREF_CHAT (purple_prefs_get_string("/plugins/core/enhanced_history/string_chat"))

PurplePluginPref *ppref;

static gboolean _scroll_imhtml_to_end(gpointer data)
{
	GtkIMHtml *imhtml = data;
	gtk_imhtml_scroll_to_end(GTK_IMHTML(imhtml), FALSE);
	g_object_unref(G_OBJECT(imhtml));
	return FALSE;
}

static void historize(PurpleConversation *c)
{
	PurpleAccount *account = purple_conversation_get_account(c);
	PurpleConversationType convtype;
	PidginConversation *gtkconv = NULL;
	GtkIMHtmlOptions options;
	GList *logs = NULL;
	GSList *buddies = NULL;
	const char *name = NULL, *alias = NULL, *LOG_MODE = NULL;
	char *header, *protocol, *history;
	guint flags;
	int size = 0;
	int counter;

	account = purple_conversation_get_account(c);
	name = purple_conversation_get_name(c);
	alias = name;
	LOG_MODE = purple_prefs_get_string("/purple/logging/format");

	/* If logging isn't enabled, don't show any history */
	if(!purple_prefs_get_bool("/purple/logging/log_ims") &&
			!purple_prefs_get_bool("/purple/logging/log_chats"))
		return;
	
	/* If the user wants to show 0 logs, stop now */
	if(NUM_OF_CHATS == 0) {
		return;
	}
	
	/* If the logging mode is html, set the output options to include no newline.
	 * Otherwise, it's normal text, so we don't need extra lines */
	if(strcasecmp(LOG_MODE,"html")==0) {
		options = GTK_IMHTML_NO_NEWLINE;
	} else {
		options = GTK_IMHTML_NO_COLOURS;
	}
	
	/* Find buddies for this conversation. */
	buddies = purple_find_buddies(account, name);

	/* If we found at least one buddy, save the first buddy's alias. */
	if (buddies != NULL)
		alias = purple_buddy_get_contact_alias((PurpleBuddy *)buddies->data);

	/* Determine whether this is an IM or a chat. In either case, if the user has that
	 * particular log type disabled, the logs file doesnt not get specified */
	convtype = purple_conversation_get_type(c);
	if (convtype == PURPLE_CONV_TYPE_IM && !strcmp(PREF_IM,"yes")) {
		logs = purple_log_get_logs(PURPLE_LOG_IM,
				purple_conversation_get_name(c), purple_conversation_get_account(c));
		logs = purple_log_get_logs(PURPLE_LOG_IM,
				purple_conversation_get_name(c), purple_conversation_get_account(c));
	} else if (convtype == PURPLE_CONV_TYPE_CHAT && !strcmp(PREF_CHAT,"yes")) {
		logs = purple_log_get_logs(PURPLE_LOG_CHAT,
			purple_conversation_get_name(c), purple_conversation_get_account(c));
	}

	/* The logs are non-existant or the user has disabled this type for log displaying. */
	if (!logs) {
		return;
	}

	gtkconv = PURPLE_GTK_CONVERSATION(c);

	size = g_list_length(logs);
	
	/* Make sure the user selected number of chats does not exceed the number of logs. */
	if(size > NUM_OF_CHATS) {
		size=NUM_OF_CHATS;
	}

	/* No idea wth this does, it was in the original history plugin */
	if (flags & PURPLE_LOG_READ_NO_NEWLINE) {
		options |= GTK_IMHTML_NO_NEWLINE;
	}

	/* Deal with time limitations */
	counter = 0;
	if(NUM_MINS ==0 && NUM_HOURS ==0 && NUM_DAYS ==0) {
		/* No time limitations, advance the logs NUM_OF_CHATS-1 forward */
		while( logs->next && counter < (NUM_OF_CHATS-1)) {
			logs = logs->next;
			counter++;
			printf("Counter: %d\n",counter);
		}
	} else {
		struct tm *log_tm, *local_tm;
		time_t t,log_time;
		double limit_time, diff_time;
		
		/* Grab current time and normalize it to UTC */
		t = time(NULL);
		local_tm = gmtime(&t);
		t = mktime(local_tm);
		
		/* Pull the local time from the purple log, convert it to UTC time */
		log_tm = gmtime(&((PurpleLog*)logs->data)->time);
		log_time = mktime(log_tm);
		printf("Local Time as int: %d \n",(int)t);
		printf("Log Time as int: %d \n",(int) mktime(log_tm));
		limit_time = NUM_MINS*60.0 + NUM_HOURS*60.0*60.0 + NUM_DAYS*60.0*60.0*24.0;
		diff_time = difftime( t, log_time );
		printf("Time difference between local and log: %.21f \n",diff_time);
		
		/* The most recent log is already too old, so lets return */
		if(diff_time > limit_time) {
			return;
		}
		/* Iterate to the end of the list, stop while messages are under limit, we just
		 * want a count here */
		while( logs->next && diff_time <= limit_time && counter < (NUM_OF_CHATS-1)) {
			logs = logs->next;
			log_tm = gmtime(&((PurpleLog*)logs->data)->time);
			log_time = mktime(log_tm);
			diff_time = difftime( t, log_time );
			counter++;
		}
		if(diff_time > limit_time) {
			logs = logs->prev;
		}
	}
		
	if(counter == 0) {
		return;
	}
	/* Loop through the logs and print them to the window */
	while(logs) {
		protocol = g_strdup(gtk_imhtml_get_protocol_name(GTK_IMHTML(gtkconv->imhtml)));
		gtk_imhtml_set_protocol_name(GTK_IMHTML(gtkconv->imhtml),purple_account_get_protocol_name(((PurpleLog*)logs->data)->account));
		
		if (gtk_text_buffer_get_char_count(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtkconv->imhtml)))) {
			gtk_imhtml_append_text(GTK_IMHTML(gtkconv->imhtml), "<BR>", options);
		}

		/* Print a header at the beginning of the log */
		header = g_strdup_printf(_("<b>Conversation with %s on %s:</b><br>"), alias,
							 purple_date_format_full(localtime(&((PurpleLog *)logs->data)->time)));
		
		if(strcmp(PREF_DATE,"yes")==0) {
			gtk_imhtml_append_text(GTK_IMHTML(gtkconv->imhtml), header, options);
		}
		g_free(header);

		/* Copy the log string into the history array */
		history = purple_log_read((PurpleLog*)logs->data, &flags);		
		
		gtk_imhtml_append_text(GTK_IMHTML(gtkconv->imhtml), history, options);
		g_free(history);
		
		/* Advance the list so that the next time through the loop we get the next log */
		logs = logs->prev;
	}
	
	gtk_imhtml_set_protocol_name(GTK_IMHTML(gtkconv->imhtml), protocol);
	g_free(protocol);
	
	g_object_ref(G_OBJECT(gtkconv->imhtml));
	g_idle_add(_scroll_imhtml_to_end, gtkconv->imhtml);
	
	/* Clear the allocated memory that the logs are using */
	g_list_foreach(logs,(GFunc)purple_log_free, NULL);
	g_list_free(logs);
	
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	purple_signal_connect(purple_conversations_get_handle(),
						"conversation-created",
						plugin, PURPLE_CALLBACK(historize), NULL);

	return TRUE;
}

static PurplePluginPrefFrame *
get_plugin_pref_frame(PurplePlugin *plugin) {
	PurplePluginPrefFrame *frame;

	frame = purple_plugin_pref_frame_new();

	/* Label for the number of chats */
	ppref = purple_plugin_pref_new_with_label("# Of Previous Conversations to Display");
	purple_plugin_pref_frame_add(frame, ppref);

	/*Integer input-box for number of chats */
	ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/enhanced_history/int","# of Chats:");
	purple_plugin_pref_set_bounds(ppref, 0, 255);
	purple_plugin_pref_frame_add(frame, ppref);

	/* Label for the date option */
	ppref = purple_plugin_pref_new_with_label("Would you like the date of the log displayed in-line with the text?");
	purple_plugin_pref_frame_add(frame, ppref);
	
	/* Drop-down selection for Date insertion */
	ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/enhanced_history/string_date","Dates:");
    purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
    purple_plugin_pref_add_choice(ppref, "yes", "yes");
    purple_plugin_pref_add_choice(ppref, "no", "no");
	purple_plugin_pref_frame_add(frame, ppref);
	
	/* Label for the log display */
	ppref = purple_plugin_pref_new_with_label("For which conversation types would you like to display the log?");
	purple_plugin_pref_frame_add(frame, ppref);

	/* Drop-down selection for IM Log displaying */
	ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/enhanced_history/string_im","Instant Messages:");
    purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
    purple_plugin_pref_add_choice(ppref, "yes", "yes");
    purple_plugin_pref_add_choice(ppref, "no", "no");
	purple_plugin_pref_frame_add(frame, ppref);

	/* Drop-down selection for Chat Log displaying */
	ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/enhanced_history/string_chat","Chat Rooms / IRC:");
    purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
    purple_plugin_pref_add_choice(ppref, "yes", "yes");
    purple_plugin_pref_add_choice(ppref, "no", "no");
	purple_plugin_pref_frame_add(frame, ppref);
	
	/* Label for maximum log age option */
	ppref = purple_plugin_pref_new_with_label("Maximum age for logs view (0 for no limit):");
	purple_plugin_pref_frame_add(frame, ppref);
	
	/* Input for minutes */
	ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/enhanced_history/mins","# Mins:");
	purple_plugin_pref_set_bounds(ppref, 0, 60);
	purple_plugin_pref_frame_add(frame, ppref);

	/* Input for hours */
	ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/enhanced_history/hours","# Hours:");
	purple_plugin_pref_set_bounds(ppref, 0, 24);
	purple_plugin_pref_frame_add(frame, ppref);
		
	/* Input for days */
	ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/enhanced_history/days","# Days:");
	purple_plugin_pref_set_bounds(ppref, 0, 255);
	purple_plugin_pref_frame_add(frame, ppref);

	
	return frame;
}

static PurplePluginUiInfo prefs_info = {
	get_plugin_pref_frame
};


static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,

	"gtk-plugin_pack-enhanced_history",
	"Enhanced History",
	VERSION,
	"An enhanced version of the history plugin.",
	"An enhanced versoin of the history plugin. Grants ability to select the number of previous conversations to show instead of just one.",
	"Andrew Pangborn <gaim@andrewpangborn.com>",
	PURPLE_WEBSITE,
	plugin_load,
	NULL,
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

static void
init_plugin(PurplePlugin *plugin)
{
	purple_prefs_add_none("/plugins/core/enhanced_history");
	purple_prefs_add_int("/plugins/core/enhanced_history/int", 3);
	purple_prefs_add_int("/plugins/core/enhanced_history/mins",0);
	purple_prefs_add_int("/plugins/core/enhanced_history/hours",0);
	purple_prefs_add_int("/plugins/core/enhanced_history/days",0);
	purple_prefs_add_string("/plugins/core/enhanced_history/string_date","yes");
	purple_prefs_add_string("/plugins/core/enhanced_history/string_im","yes");
	purple_prefs_add_string("/plugins/core/enhanced_history/string_chat","yes");

}

PURPLE_INIT_PLUGIN(enhanced_history, init_plugin, info)


/*
 * Release Notification Plugin
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

 /* DESCRIPTION: Puts log of past conversations in the user's IM window
 * Allows the user to select the number of conversations to display from
 * gaim plugin preferences.
 *
 * Created: 02/11/2005
 * Last Updated: 05/07/2007
 *
 * Change Log
 *     1.2:
 *          Made a version that compiles with pidgin 2.0.0 
 *     1.1:
 *     	   Apparently local times weren't neccesarily being converted to UTC before
 *     	   being compared to the log time in linux.
 *     1.0:
 *         Fixed time limitations. Changed the way the conversation timestamp/header
 *         is printed (copied how its done from the history.c that comes with gaim).
 *     0.9:
 *         Changed the plugin preference paths so they aren't stomping on the plugin
 *         example anymore. Large overhaul on how the program is coded. 
 *         Making use of iterators rather than manual loop conditions.
 *         
 *          Added the ability to specify the age of the logs viewed. 
 *          i.e. if the user selects (30 mins and 1 hour), Only logs dated less than 90 mins
 *          from the current system time will be shown. The smaller of the two constraints
 *          (# of chats / time constraints) will determine the number of logs shown.
 *     0.8:
 *         Changed the way dates of conversations are displayed.
 *         Removed the need for the user to select the loggin mode.
 *     0.7:
 *         Changed the names of the conversation type constants to work with gaim 2.0
 *     0.6:
 *         Added the option to display inline dates from the log files.
 *         Fixed a memory leak. Commented the code significantly more.
 *     0.5:
 *         Added the option to disable log display for IMs and chats independently.
 *     0.4:
 *         The data size limitation was pretty much worthless, so I removed that.
 *         I changed the Options flag for how the IM html prints, so that it has colors.
 *     0.3:
 *         Added weak implementation to limit the chats based on data size.
 *     0.2:
 *         Added GUI/preferences window for gaim.
 *     0.1:
 *         Initial revision, allows static number of past conversations
 *         to be displayed. (Only changed by recompiling yourself).
 *
 * Author: 
 *         Andrew Pangborn  - gaim@andrewpangborn.com
 *
 * Contributors:
 *         Ankit Singla
 */

#include "gaim-compat.h"
#include "gtkgaim-compat.h"

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef GAIM_PLUGINS
# define GAIM_PLUGINS
#endif

#ifndef GAIM_WEBSITE
#define GAIM_WEBSITE PURPLE_WEBSITE
#endif

#include "prpl.h"
#include "internal.h"
#include "pidgin.h"

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

#define NUM_OF_CHATS (gaim_prefs_get_int("/plugins/core/enhanced_history/int"))
#define NUM_MINS (gaim_prefs_get_int("/plugins/core/enhanced_history/mins"))
#define NUM_HOURS (gaim_prefs_get_int("/plugins/core/enhanced_history/hours"))
#define NUM_DAYS (gaim_prefs_get_int("/plugins/core/enhanced_history/days"))
#define	PREF_DATE (gaim_prefs_get_string("/plugins/core/enhanced_history/string_date"))
#define	PREF_IM (gaim_prefs_get_string("/plugins/core/enhanced_history/string_im"))
#define PREF_CHAT (gaim_prefs_get_string("/plugins/core/enhanced_history/string_chat"))

GaimPluginPref *ppref;

static gboolean _scroll_imhtml_to_end(gpointer data)
{
	GtkIMHtml *imhtml = data;
	gtk_imhtml_scroll_to_end(GTK_IMHTML(imhtml), FALSE);
	g_object_unref(G_OBJECT(imhtml));
	return FALSE;
}

static void historize(GaimConversation *c)
{
	GaimAccount *account = gaim_conversation_get_account(c);
	const char *name = gaim_conversation_get_name(c);
	GaimConversationType convtype;
	GList *logs = NULL;
	const char *alias = name;
	GaimGtkConversation *gtkconv;
	guint flags;
	GtkIMHtmlOptions options;
	char *header;
	char *protocol;
	char* history;
	int size = 0;
	int counter;
	const char *LOG_MODE = gaim_prefs_get_string("/purple/logging/format");
	GSList *buddies;
	

	// If logging isn't enabled, don't show any history
	if(!gaim_prefs_get_bool("/purple/logging/log_ims") && !gaim_prefs_get_bool("/purple/logging/log_chats")) {
		return;
	}
	
	/* If the user wants to show 0 logs, stop now */
	if(NUM_OF_CHATS == 0) {
		return;
	}
	
	// If the logging mode is html, set the output options to include no newline.
	// Otherwise, it's normal text, so we don't need extra lines
	if(strcasecmp(LOG_MODE,"html")==0) {
		options = GTK_IMHTML_NO_NEWLINE;
	} else {
		options = GTK_IMHTML_NO_COLOURS;
	}
	
	/* Find buddies for this conversation. */
	buddies = gaim_find_buddies(account, name);

	/* If we found at least one buddy, save the first buddy's alias. */
	if (buddies != NULL)
		alias = gaim_buddy_get_contact_alias((GaimBuddy *)buddies->data);

	/* Determine whether this is an IM or a chat. In either case, if the user has that particular log type
	    disabled, the logs file doesnt not get specified */
	convtype = gaim_conversation_get_type(c);
	if (convtype == GAIM_CONV_TYPE_IM && !strcmp(PREF_IM,"yes")) {
		logs = gaim_log_get_logs(GAIM_LOG_IM,
				gaim_conversation_get_name(c), gaim_conversation_get_account(c));
		logs = gaim_log_get_logs(GAIM_LOG_IM,
				gaim_conversation_get_name(c), gaim_conversation_get_account(c));
	} else if (convtype == GAIM_CONV_TYPE_CHAT && !strcmp(PREF_CHAT,"yes")) {
		logs = gaim_log_get_logs(GAIM_LOG_CHAT,
			gaim_conversation_get_name(c), gaim_conversation_get_account(c));
	}

	// The logs are non-existant or the user has disabled this type for log displaying.
	if (!logs) {
		return;
	}

	gtkconv = GAIM_GTK_CONVERSATION(c);

	size = g_list_length(logs);
	
	/* Make sure the user selected number of chats does not exceed the number of logs. */
	if(size > NUM_OF_CHATS) {
		size=NUM_OF_CHATS;
	}

	// No idea wth this does, it was in the original history plugin
	if (flags & GAIM_LOG_READ_NO_NEWLINE) {
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
		
		/* Pull the local time from the gaim log, convert it to UTC time */
		log_tm = gmtime(&((GaimLog*)logs->data)->time);
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
		//Iterate to the end of the list, stop while messages are under limit, we just want a count here
		while( logs->next && diff_time <= limit_time && counter < (NUM_OF_CHATS-1)) {
			logs = logs->next;
			log_tm = gmtime(&((GaimLog*)logs->data)->time);
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
		gtk_imhtml_set_protocol_name(GTK_IMHTML(gtkconv->imhtml),gaim_account_get_protocol_name(((GaimLog*)logs->data)->account));
		
		if (gtk_text_buffer_get_char_count(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtkconv->imhtml)))) {
			gtk_imhtml_append_text(GTK_IMHTML(gtkconv->imhtml), "<BR>", options);
		}

		/* Print a header at the beginning of the log */
		header = g_strdup_printf(_("<b>Conversation with %s on %s:</b><br>"), alias,
							 gaim_date_format_full(localtime(&((GaimLog *)logs->data)->time)));
		
		if(strcmp(PREF_DATE,"yes")==0) {
			gtk_imhtml_append_text(GTK_IMHTML(gtkconv->imhtml), header, options);
		}
		g_free(header);

		/* Copy the log string into the history array */
		history = gaim_log_read((GaimLog*)logs->data, &flags);		
		
		gtk_imhtml_append_text(GTK_IMHTML(gtkconv->imhtml), history, options);
		g_free(history);
		
		/* Advance the list so that the next time through the loop we get the next log */
		logs = logs->prev;
	}
	
	gtk_imhtml_set_protocol_name(GTK_IMHTML(gtkconv->imhtml), protocol);
	g_free(protocol);
	
	g_object_ref(G_OBJECT(gtkconv->imhtml));
	g_idle_add(_scroll_imhtml_to_end, gtkconv->imhtml);
	
	// Clear the allocated memory that the logs are using
	g_list_foreach(logs,(GFunc)gaim_log_free, NULL);
	g_list_free(logs);
	
}

static gboolean
plugin_load(GaimPlugin *plugin)
{
	gaim_signal_connect(gaim_conversations_get_handle(),
						"conversation-created",
						plugin, GAIM_CALLBACK(historize), NULL);

	return TRUE;
}

static GaimPluginPrefFrame *
get_plugin_pref_frame(GaimPlugin *plugin) {
	GaimPluginPrefFrame *frame;

	frame = gaim_plugin_pref_frame_new();

	//Label for the number of chats
	ppref = gaim_plugin_pref_new_with_label("# Of Previous Conversations to Display");
	gaim_plugin_pref_frame_add(frame, ppref);

	//Integer input-box for number of chats
	ppref = gaim_plugin_pref_new_with_name_and_label("/plugins/core/enhanced_history/int","# of Chats:");
	gaim_plugin_pref_set_bounds(ppref, 0, 255);
	gaim_plugin_pref_frame_add(frame, ppref);

	//Label for the date option
	ppref = gaim_plugin_pref_new_with_label("Would you like the date of the log displayed in-line with the text?");
	gaim_plugin_pref_frame_add(frame, ppref);
	
	//Drop-down selection for Date insertion
	ppref = gaim_plugin_pref_new_with_name_and_label("/plugins/core/enhanced_history/string_date","Dates:");
		    gaim_plugin_pref_set_type(ppref, GAIM_PLUGIN_PREF_CHOICE);
		    gaim_plugin_pref_add_choice(ppref, "yes", "yes");
		    gaim_plugin_pref_add_choice(ppref, "no", "no");
	gaim_plugin_pref_frame_add(frame, ppref);
	
	//Label for the log display
	ppref = gaim_plugin_pref_new_with_label("For which conversation types would you like to display the log?");
	gaim_plugin_pref_frame_add(frame, ppref);

	//Drop-down selection for IM Log displaying
	ppref = gaim_plugin_pref_new_with_name_and_label("/plugins/core/enhanced_history/string_im","Instant Messages:");
		    gaim_plugin_pref_set_type(ppref, GAIM_PLUGIN_PREF_CHOICE);
		    gaim_plugin_pref_add_choice(ppref, "yes", "yes");
		    gaim_plugin_pref_add_choice(ppref, "no", "no");
	gaim_plugin_pref_frame_add(frame, ppref);

	//Drop-down selection for Chat Log displaying
	ppref = gaim_plugin_pref_new_with_name_and_label("/plugins/core/enhanced_history/string_chat","Chat Rooms / IRC:");
		    gaim_plugin_pref_set_type(ppref, GAIM_PLUGIN_PREF_CHOICE);
		    gaim_plugin_pref_add_choice(ppref, "yes", "yes");
		    gaim_plugin_pref_add_choice(ppref, "no", "no");
	gaim_plugin_pref_frame_add(frame, ppref);
	
	//Label for maximum log age option
	ppref = gaim_plugin_pref_new_with_label("Maximum age for logs view (0 for no limit):");
	gaim_plugin_pref_frame_add(frame, ppref);
	
	//Input for minutes
	ppref = gaim_plugin_pref_new_with_name_and_label("/plugins/core/enhanced_history/mins","# Mins:");
	gaim_plugin_pref_set_bounds(ppref, 0, 60);
	gaim_plugin_pref_frame_add(frame, ppref);

	//Input for hours
	ppref = gaim_plugin_pref_new_with_name_and_label("/plugins/core/enhanced_history/hours","# Hours:");
	gaim_plugin_pref_set_bounds(ppref, 0, 24);
	gaim_plugin_pref_frame_add(frame, ppref);
		
	//Input for days
	ppref = gaim_plugin_pref_new_with_name_and_label("/plugins/core/enhanced_history/days","# Days:");
	gaim_plugin_pref_set_bounds(ppref, 0, 255);
	gaim_plugin_pref_frame_add(frame, ppref);

	
	return frame;
}

static GaimPluginUiInfo prefs_info = {
	get_plugin_pref_frame
};


static GaimPluginInfo info =
{
	GAIM_PLUGIN_MAGIC,
	GAIM_MAJOR_VERSION,
	GAIM_MINOR_VERSION,
	GAIM_PLUGIN_STANDARD,                             /**< type           */
	NULL,                                             /**< ui_requirement */
	0,                                                /**< flags          */
	NULL,                                             /**< dependencies   */
	GAIM_PRIORITY_DEFAULT,                            /**< priority       */

	"core-enhanced_history",                     /**< id             */
	"Enhanced_History",                           /**< name           */
	VERSION,                                          /**< version        */
	                                                  /**  summary        */
	"An enhanced version of the history plugin.",
	                                                  /**  description    */
	"An enhanced versoin of the history plugin. Grants ability to select the number of previous conversations to show instead of just one.",
	"Andrew Pangborn <gaim@andrewpangborn.com>",      /**< author         */
	GAIM_WEBSITE,                                     /**< homepage       */

	plugin_load,                                             /**< load           */
	NULL,                                             /**< unload         */
	NULL,                                             /**< destroy        */

	NULL,                                             /**< ui_info        */
	NULL,                                             /**< extra_info     */
	&prefs_info,                                      /**< prefs_info     */
	NULL
};

static void
init_plugin(GaimPlugin *plugin)
{
	gaim_prefs_add_none("/plugins/core/enhanced_history");
	gaim_prefs_add_int("/plugins/core/enhanced_history/int", 3);
	gaim_prefs_add_int("/plugins/core/enhanced_history/int2",5);
	gaim_prefs_add_int("/plugins/core/enhanced_history/mins",0);
	gaim_prefs_add_int("/plugins/core/enhanced_history/hours",0);
	gaim_prefs_add_int("/plugins/core/enhanced_history/days",0);
	gaim_prefs_add_string("/plugins/core/enhanced_history/string_date","yes");
	gaim_prefs_add_string("/plugins/core/enhanced_history/string_im","yes");
	gaim_prefs_add_string("/plugins/core/enhanced_history/string_chat","yes");

}

GAIM_INIT_PLUGIN(enhanced_history, init_plugin, info)







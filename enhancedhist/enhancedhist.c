/*
 * enhanced_hist.c - Enhanced History Plugin for libpurple
 * Copyright (C) 2004-2008 Andrew Pangborn <gaim@andrewpangborn.com>
 * Copyright (C) 2007-2008 John Bailey <rekkanoryo@rekkanoryo.org>
 * Copyright (C) 2007 Ankit Singla
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

/* If you can't figure out what this line is for, DON'T TOUCH IT. */
#include "../common/pp_internal.h"

/* Purple headers */
#include <conversation.h>
#include <debug.h>
#include <log.h>
#include <plugin.h>
#include <pluginpref.h>
#include <prefs.h>
#include <signals.h>
#include <util.h>

/* Pidgin headers */
#include <gtkconv.h>
#include <gtkimhtml.h>
#include <gtkplugin.h>
#include <gtkprefs.h>
#include <gtkutils.h>
#include <pidgin.h>

/* libc headers */
#include <time.h>

#define ENHANCED_HISTORY_ID "gtk-plugin_pack-enhanced_history"

#define PREF_ROOT_PATH		"/plugins/gtk/plugin_pack/enhanced_history"
#define PREF_NUMBER_PATH	"/plugins/gtk/plugin_pack/enhanced_history/number"
#define PREF_MINS_PATH		"/plugins/gtk/plugin_pack/enhanced_history/minutes"
#define PREF_HOURS_PATH		"/plugins/gtk/plugin_pack/enhanced_history/hours"
#define PREF_DAYS_PATH		"/plugins/gtk/plugin_pack/enhanced_history/days"
#define PREF_DATES_PATH		"/plugins/gtk/plugin_pack/enhanced_history/dates"
#define PREF_IM_PATH		"/plugins/gtk/plugin_pack/enhanced_history/im"
#define PREF_CHAT_PATH		"/plugins/gtk/plugin_pack/enhanced_history/chat"

#define PREF_NUMBER_VAL		purple_prefs_get_int(PREF_NUMBER_PATH)
#define PREF_MINS_VAL		purple_prefs_get_int(PREF_MINS_PATH)
#define PREF_HOURS_VAL		purple_prefs_get_int(PREF_HOURS_PATH)
#define PREF_DAYS_VAL		purple_prefs_get_int(PREF_DAYS_PATH)
#define PREF_DATES_VAL		purple_prefs_get_bool(PREF_DATES_PATH)
#define PREF_IM_VAL			purple_prefs_get_bool(PREF_IM_PATH)
#define PREF_CHAT_VAL		purple_prefs_get_bool(PREF_CHAT_PATH)

static gboolean _scroll_imhtml_to_end(gpointer data)
{
	GtkIMHtml *imhtml = data;
	gtk_imhtml_scroll_to_end(GTK_IMHTML(imhtml), FALSE);
	g_object_unref(G_OBJECT(imhtml));
	return FALSE;
}

static void historize(PurpleConversation *c)
{
	PurpleAccount *account = NULL;
	PurpleConversationType convtype;
	PidginConversation *gtkconv = NULL;
	GtkIMHtmlOptions options;
	GList *logs = NULL;
	GSList *buddies = NULL;
	const char *name = NULL, *alias = NULL, *LOG_MODE = NULL;
	char *header = NULL, *protocol = NULL, *history = NULL;
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
	if(PREF_NUMBER_VAL == 0) {
		return;
	}
	
	/* If the logging mode is html, set the output options to include no newline.
	 * Otherwise, it's normal text, so we don't need extra lines */
	if(strcasecmp(LOG_MODE, "html")==0) {
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
	if (convtype == PURPLE_CONV_TYPE_IM && PREF_IM_VAL) {
		logs = purple_log_get_logs(PURPLE_LOG_IM,
				purple_conversation_get_name(c), purple_conversation_get_account(c));
	} else if (convtype == PURPLE_CONV_TYPE_CHAT && PREF_CHAT_VAL) {
		logs = purple_log_get_logs(PURPLE_LOG_CHAT,
			purple_conversation_get_name(c), purple_conversation_get_account(c));
	}

	/* The logs are non-existant or the user has disabled this type for log displaying. */
	if (!logs) {
		return;
	}

	gtkconv = PIDGIN_CONVERSATION(c);

	size = g_list_length(logs);
	
	/* Make sure the user selected number of chats does not exceed the number of logs. */
	if(size > PREF_NUMBER_VAL) {
		size=PREF_NUMBER_VAL;
	}

	/* No idea wth this does, it was in the original history plugin */
	if (flags & PURPLE_LOG_READ_NO_NEWLINE) {
		options |= GTK_IMHTML_NO_NEWLINE;
	}

	/* Deal with time limitations */
	counter = 0;
	if(PREF_MINS_VAL == 0 && PREF_HOURS_VAL == 0 && PREF_DAYS_VAL == 0) {
		/* No time limitations, advance the logs PREF_NUMBER_VAL-1 forward */
		while(logs->next && counter < (PREF_NUMBER_VAL - 1)) {
			logs = logs->next;
			counter++;
			purple_debug_info("ehnahcedhist", "Counter: %d\n", counter);
		}
	} else {
		struct tm *log_tm = NULL, *local_tm = NULL;
		time_t t, log_time;
		double limit_time, diff_time;
		
		/* Grab current time and normalize it to UTC */
		t = time(NULL);
		local_tm = gmtime(&t);
		t = mktime(local_tm);
		
		/* Pull the local time from the purple log, convert it to UTC time */
		log_tm = gmtime(&((PurpleLog*)logs->data)->time);
		log_time = mktime(log_tm);

		purple_debug_info("enhancedhist", "Local Time as int: %d \n", (int)t);
		purple_debug_info("enhancedhist", "Log Time as int: %d \n", (int)mktime(log_tm));

		limit_time = (PREF_MINS_VAL * 60.0) + (PREF_HOURS_VAL * 60.0 * 60.0) +
				(PREF_DAYS_VAL * 60.0 * 60.0 * 24.0);
		diff_time = difftime(t, log_time);
		purple_debug_info("enhancedhist", "Time difference between local and log: %.21f \n",
				diff_time);
		
		/* The most recent log is already too old, so lets return */
		if(diff_time > limit_time) {
			return;
		}
		/* Iterate to the end of the list, stop while messages are under limit, we just
		 * want a count here */
		while(logs->next && diff_time <= limit_time && counter < (PREF_NUMBER_VAL - 1)) {
			logs = logs->next;
			log_tm = gmtime(&((PurpleLog*)logs->data)->time);
			log_time = mktime(log_tm);
			diff_time = difftime(t, log_time);
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
		gtk_imhtml_set_protocol_name(GTK_IMHTML(gtkconv->imhtml),
			purple_account_get_protocol_name(((PurpleLog*)logs->data)->account));
		
		if (gtk_text_buffer_get_char_count(gtk_text_view_get_buffer(
				GTK_TEXT_VIEW(gtkconv->imhtml))))
		{
			gtk_imhtml_append_text(GTK_IMHTML(gtkconv->imhtml), "<BR>", options);
		}

		/* Print a header at the beginning of the log */
		header = g_strdup_printf(_("<b>Conversation with %s on %s:</b><br>"), alias,
				purple_date_format_full(localtime(&((PurpleLog *)logs->data)->time)));
		
		if(PREF_DATES_VAL) {
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
	g_list_foreach(logs, (GFunc)purple_log_free, NULL);
	g_list_free(logs);
	
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	purple_signal_connect(purple_conversations_get_handle(),
			"conversation-created", plugin, PURPLE_CALLBACK(historize), NULL);

	return TRUE;
}

static GtkWidget *
eh_prefs_get_frame(PurplePlugin *plugin)
{
	GtkSizeGroup *sg = NULL;
	GtkWidget *vbox = NULL, *frame = NULL, *option = NULL;

	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	vbox = gtk_vbox_new(TRUE, PIDGIN_HIG_BOX_SPACE);

	/* heading for the more general options */
	frame = pidgin_make_frame(vbox, _("Display Options"));

	/* the integer pref for the number of logs to display */
	pidgin_prefs_labeled_spin_button(frame, _("Number of previous conversations to display:"),
			PREF_NUMBER_PATH, 0, 255, NULL);

	/* the boolean preferences */
	option = pidgin_prefs_checkbox(_("Show dates with text"), PREF_DATES_PATH, frame);
	option = pidgin_prefs_checkbox(_("Show logs for IMs"), PREF_IM_PATH, frame);
	option = pidgin_prefs_checkbox(_("Show logs for chats"), PREF_CHAT_PATH, frame);

	/* heading for the age limit options */
	frame = pidgin_make_frame(vbox, _("Age Limit for Logs (0 to disable):"));

	/* the integer preferences for time limiting */
	option = pidgin_prefs_labeled_spin_button(frame, "Days:", PREF_DAYS_PATH, 0, 255, sg);
	option = pidgin_prefs_labeled_spin_button(frame, "Hours:", PREF_HOURS_PATH, 0, 255, sg);
	option = pidgin_prefs_labeled_spin_button(frame, "Minutes:", PREF_MINS_PATH, 0, 255, sg);

	gtk_widget_show_all(vbox);

	return vbox;
}


static PidginPluginUiInfo ui_info = {
	eh_prefs_get_frame, /* get prefs frame */
	0, /* page number - reserved */

	/* reserved pointers */
	NULL,
	NULL,
	NULL,
	NULL
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

	ENHANCED_HISTORY_ID,
	NULL,
	PP_VERSION,
	NULL,
	NULL,

	"Andrew Pangborn <gaim@andrewpangborn.com>",
	PP_WEBSITE,

	plugin_load,
	NULL,
	NULL, 

	&ui_info,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin *plugin)
{
	gboolean dates = FALSE, ims = FALSE, chats = FALSE;

	purple_prefs_add_none(PREF_ROOT_PATH);

	if(purple_prefs_exists("/plugins/core/enhanced_history/int")) {
		/* Rename these prefs to fit within the Plugin Pack scheme */
		purple_prefs_rename("/plugins/core/enhanced_history/int", PREF_NUMBER_PATH);
		purple_prefs_rename("/plugins/core/enhanced_history/mins", PREF_MINS_PATH);
		purple_prefs_rename("/plugins/core/enhanced_history/hours", PREF_HOURS_PATH);
		purple_prefs_rename("/plugins/core/enhanced_history/days", PREF_DAYS_PATH);

		if(strcmp(purple_prefs_get_string("/plugins/core/enhanced_history/string_date"), "no"))
			dates = TRUE;
		if(strcmp(purple_prefs_get_string("/plugins/core/enhanced_history/string_im"), "no"))
			ims = TRUE;
		if(strcmp(purple_prefs_get_string("/plugins/core/enhanced_history/string_chat"), "no"))
			chats = TRUE;

		purple_prefs_remove("/plugins/core/enhanced_history/string_date");
		purple_prefs_remove("/plugins/core/enhanced_history/string_im");
		purple_prefs_remove("/plugins/core/enhanced_history/string_chat");
		purple_prefs_remove("/plugins/core/enhanced_history");

		purple_prefs_add_bool(PREF_DATES_PATH, dates);
		purple_prefs_add_bool(PREF_IM_PATH, ims);
		purple_prefs_add_bool(PREF_CHAT_PATH, chats);
	} else {
		/* Create these prefs with sensible defaults */
		purple_prefs_add_int(PREF_NUMBER_PATH, 1);
		purple_prefs_add_int(PREF_MINS_PATH, 0);
		purple_prefs_add_int(PREF_HOURS_PATH, 0);
		purple_prefs_add_int(PREF_DAYS_PATH, 0);
		purple_prefs_add_bool(PREF_DATES_PATH, TRUE);
		purple_prefs_add_bool(PREF_IM_PATH, TRUE);
		purple_prefs_add_bool(PREF_CHAT_PATH, FALSE);
	}

	info.name = _("Enhanced History");
	info.summary = _("An enhanced version of the history plugin.");
	info.description = _("An enhanced versoin of the history plugin. Grants ability to "
			"select the number of previous conversations to show instead of just one.");
}

PURPLE_INIT_PLUGIN(enhanced_history, init_plugin, info)


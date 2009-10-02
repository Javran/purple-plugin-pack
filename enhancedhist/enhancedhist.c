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

#define PREF_ROOT_GPPATH	"/plugins/gtk"
#define PREF_ROOT_PPATH		"/plugins/gtk/plugin_pack"
#define PREF_ROOT_PATH		"/plugins/gtk/plugin_pack/enhanced_history"
#define PREF_NUMBER_PATH	"/plugins/gtk/plugin_pack/enhanced_history/number"
#define PREF_BYTES_PATH		"/plugins/gtk/plugin_pack/enhanced_history/bytes"
#define PREF_MINS_PATH		"/plugins/gtk/plugin_pack/enhanced_history/minutes"
#define PREF_HOURS_PATH		"/plugins/gtk/plugin_pack/enhanced_history/hours"
#define PREF_DAYS_PATH		"/plugins/gtk/plugin_pack/enhanced_history/days"
#define PREF_DATES_PATH		"/plugins/gtk/plugin_pack/enhanced_history/dates"
#define PREF_IM_PATH		"/plugins/gtk/plugin_pack/enhanced_history/im"
#define PREF_CHAT_PATH		"/plugins/gtk/plugin_pack/enhanced_history/chat"

#define PREF_NUMBER_VAL		purple_prefs_get_int(PREF_NUMBER_PATH)
#define PREF_BYTES_VAL		purple_prefs_get_int(PREF_BYTES_PATH)
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
	GtkTextIter start;
	GtkIMHtmlOptions options;
	GList *logs = NULL, *logs_head = NULL;
	GSList *buddies = NULL;
	struct tm *log_tm = NULL, *local_tm = NULL;
	time_t t, log_time;
	double limit_time = 0.0;
	const char *name = NULL, *alias = NULL, *LOG_MODE = NULL;
	char *header = NULL, *protocol = NULL, *history = NULL;
	guint flags;
	int conv_counter = 0;
	int limit_offset = 0;
	int byte_counter = 0;
	int check_time = 0;
	int log_size, overshoot;

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
		GSList *cur;

		for(cur = buddies; cur; cur = cur->next) {
			PurpleBlistNode *node = cur->data;

			if(node && (node->prev || node->next)) {
				PurpleBlistNode *node2;

				for(node2 = node->parent->child; node2; node2 = node2->next) {
					logs = g_list_concat(purple_log_get_logs(PURPLE_LOG_IM,
								purple_buddy_get_name((PurpleBuddy *)node2),
								purple_buddy_get_account((PurpleBuddy *)node2)), logs);
				}

				break;
			}
		}

			if(logs)
				logs = g_list_sort(logs, purple_log_compare);
			else
				logs = purple_log_get_logs(PURPLE_LOG_IM, name, account);

	} else if (convtype == PURPLE_CONV_TYPE_CHAT && PREF_CHAT_VAL) {
		logs = purple_log_get_logs(PURPLE_LOG_CHAT,
			purple_conversation_get_name(c), purple_conversation_get_account(c));
	}

	gtkconv = PIDGIN_CONVERSATION(c);

	/* The logs are non-existant or the user has disabled this type for log displaying. */
	if (!logs) {
		return;
	}

	/* Keep a pointer to the head of the logs for freeing later */
	logs_head = logs;

	/* If all time prefs are not 0, prepare to check times */
	if (!(PREF_MINS_VAL == 0 && PREF_HOURS_VAL == 0 && PREF_DAYS_VAL == 0)) {
		check_time = 1;

		/* Grab current time and normalize it to UTC */
		t = time(NULL);
		local_tm = gmtime(&t);
		t = mktime(local_tm);

		limit_time = (PREF_MINS_VAL * 60.0) + (PREF_HOURS_VAL * 60.0 * 60.0) +
				(PREF_DAYS_VAL * 60.0 * 60.0 * 24.0);
	}

	/* The protocol will need to be adjusted for each log for correct display,
	   so save the current imhtml protocol_name to restore it later */
	protocol = g_strdup(gtk_imhtml_get_protocol_name(GTK_IMHTML(gtkconv->imhtml)));

	/* Calculate time for the first log */
	log_tm = gmtime(&((PurpleLog*)logs->data)->time);
	log_time = mktime(log_tm);

	/* Continue to add older logs until they run out or the conditions are no
	   longer met */
	while (logs && conv_counter < PREF_NUMBER_VAL
			&& byte_counter < PREF_BYTES_VAL
			&& (!check_time || difftime(t, log_time) < limit_time)) {

		/* Get the current log's contents as a char* */
		history = purple_log_read((PurpleLog*)logs->data, &flags);
		log_size = strlen(history);

		/* Update the overall byte count and determine if this log exceeds the limit */
		byte_counter += log_size;
		overshoot = byte_counter - PREF_BYTES_VAL;
		if (overshoot > 0) {
			/* Start looking at the maximum log size for a newline to break at */
			limit_offset = overshoot;
			/* Find the next \n, or stop if the end of the log is reached */
			while (history[limit_offset] && history[limit_offset] != '\n') {
				limit_offset++;
			}
			/* If we're at or very close to the end of the log, forget this log */
			if (!history[limit_offset] || (log_size - limit_offset < 3)) {
				limit_offset = -1;
			}
			else {
				/* Start at the first character after the newline */
				limit_offset++;
			}
		}

		conv_counter++;

		/* If this log won't fit at all, don't display it in the conversation */
		if (limit_offset != -1) {
			/* Set the correct protocol_name for this log */
			gtk_imhtml_set_protocol_name(GTK_IMHTML(gtkconv->imhtml),
					purple_account_get_protocol_name(((PurpleLog*)logs->data)->account));

			/* Prepend the contents of the log starting at the calculated offset */
			gtk_text_buffer_get_iter_at_offset(GTK_IMHTML(gtkconv->imhtml)->text_buffer,
					&start, 0);
			gtk_imhtml_insert_html_at_iter(GTK_IMHTML(gtkconv->imhtml),
						history + limit_offset, options, &start);

			/* Prepend the conversation header */
			if (PREF_DATES_VAL) {
				header = g_strdup_printf(_("<b>Conversation with %s on %s:</b><br>"), alias,
					purple_date_format_full(localtime(&((PurpleLog *)logs->data)->time)));
				gtk_text_buffer_get_iter_at_offset(GTK_IMHTML(gtkconv->imhtml)->text_buffer,
						&start, 0);
				gtk_imhtml_insert_html_at_iter(GTK_IMHTML(gtkconv->imhtml),
						header, options, &start);
				g_free(header);
			}
		}

		g_free(history);

		if (limit_offset > 0) {
			/* This log had to be chopped to fit, so stop after this one */
			break;
		}

		logs = logs->next;

		/* Recalculate log time if we haven't run out of logs */
		if (logs) {
			log_tm = gmtime(&((PurpleLog*)logs->data)->time);
			log_time = mktime(log_tm);
		}
	}

	gtk_imhtml_append_text(GTK_IMHTML(gtkconv->imhtml), "<hr>", options);

	/* Restore the original protocol_name */
	gtk_imhtml_set_protocol_name(GTK_IMHTML(gtkconv->imhtml), protocol);
	g_free(protocol);
	
	g_object_ref(G_OBJECT(gtkconv->imhtml));
	g_idle_add(_scroll_imhtml_to_end, gtkconv->imhtml);
	
	/* Clear the allocated memory that the logs are using */
	g_list_foreach(logs_head, (GFunc)purple_log_free, NULL);
	g_list_free(logs_head);
	
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
	pidgin_prefs_labeled_spin_button(frame, _("Maximum number of conversations:"),
			PREF_NUMBER_PATH, 0, 255, NULL);

	/* the integer pref for maximum number of bytes to read back */
	pidgin_prefs_labeled_spin_button(frame, _("Maximum number of bytes:"),
			PREF_BYTES_PATH, 0, 1024*1024, NULL);

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

	purple_prefs_add_none(PREF_ROOT_GPPATH);
	purple_prefs_add_none(PREF_ROOT_PPATH);
	purple_prefs_add_none(PREF_ROOT_PATH);

	if(purple_prefs_exists("/plugins/core/enhanced_history/int")) {
		if(strcmp(purple_prefs_get_string("/plugins/core/enhanced_history/string_date"), "no"))
			dates = TRUE;
		if(strcmp(purple_prefs_get_string("/plugins/core/enhanced_history/string_im"), "no"))
			ims = TRUE;
		if(strcmp(purple_prefs_get_string("/plugins/core/enhanced_history/string_chat"), "no"))
			chats = TRUE;

		purple_prefs_add_int(PREF_NUMBER_PATH, purple_prefs_get_int("/plugins/core/enhanced_history/int"));
		purple_prefs_add_int(PREF_BYTES_PATH, purple_prefs_get_int("/plugins/core/enhanced_history/bytes"));
		purple_prefs_add_int(PREF_MINS_PATH, purple_prefs_get_int("/plugins/core/enhanced_history/mins"));
		purple_prefs_add_int(PREF_HOURS_PATH, purple_prefs_get_int("/plugins/core/enhanced_history/hours"));
		purple_prefs_add_int(PREF_DAYS_PATH, purple_prefs_get_int("/plugins/core/enhanced_history/days"));
		purple_prefs_add_bool(PREF_DATES_PATH, dates);
		purple_prefs_add_bool(PREF_IM_PATH, ims);
		purple_prefs_add_bool(PREF_CHAT_PATH, chats);

		purple_prefs_remove("/plugins/core/enhanced_history/int");
		purple_prefs_remove("/plugins/core/enhanced_history/bytes");
		purple_prefs_remove("/plugins/core/enhanced_history/mins");
		purple_prefs_remove("/plugins/core/enhanced_history/hours");
		purple_prefs_remove("/plugins/core/enhanced_history/days");
		purple_prefs_remove("/plugins/core/enhanced_history/string_date");
		purple_prefs_remove("/plugins/core/enhanced_history/string_im");
		purple_prefs_remove("/plugins/core/enhanced_history/string_chat");
		purple_prefs_remove("/plugins/core/enhanced_history");
	} else {
		/* Create these prefs with sensible defaults */
		purple_prefs_add_int(PREF_NUMBER_PATH, 10);
		purple_prefs_add_int(PREF_BYTES_PATH, 4096);
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


/*
 * Adds a command to return the first url for a google I'm feeling lucky search
 * Copyright (C) 2008 Gary Kramlich <grim@reaperworld.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301, USA.
 */

#include "../common/pp_internal.h"

#include <stdlib.h>
#include <time.h>

#include <accountopt.h>
#include <blist.h>
#include <debug.h>
#include <plugin.h>
#include <prpl.h>
#include <status.h>
#include <util.h>

#define STRESS_BUDDY(buddy) \
	((StressBuddy *)purple_buddy_get_protocol_data(buddy))

/******************************************************************************
 * Structs
 *****************************************************************************/
typedef struct {
	PurpleBuddy *buddy;
	guint timer_id;
	gint nevents;
	gint maxevents;
} StressBuddy;

/******************************************************************************
 * Enums
 *****************************************************************************/
enum {
	STRESS_EVENT_SIGN_ON,
	STRESS_EVENT_SIGN_OFF,
	STRESS_EVENT_IDLE,
	STRESS_EVENT_UNIDLE,
	STRESS_EVENT_AWAY,
	STRESS_EVENT_BACK,
	STRESS_EVENT_TYPING,
	STRESS_EVENT_STOPPED_TYPING,
	STRESS_EVENT_SEND_MESSAGE,
};

/******************************************************************************
 * Globals
 *****************************************************************************/
static GList *buddies = NULL;
static GList *events = NULL;
static gint nevents = 0;
static gint message_min = 0;
static gint message_max = 0;

/******************************************************************************
 * helpers
 *****************************************************************************/
static inline void
stress_send_im(PurpleAccount *account, PurpleBuddy *buddy, const gchar *name) {
	PurpleConnection *pc = NULL;
	GString *msg = NULL;
	gint length = 0, i = 0;

	/* build the message */
	msg = g_string_new("");
	length = (rand() % (message_max - message_min)) + message_min;

	for(i = 0; i < length; i += 4) {
		gint value = rand() % 65536;

		g_string_append_printf(msg, "%04x", value);
	}

	/* send the im */
	pc = purple_account_get_connection(account);
	serv_got_im(pc, name, msg->str, 0, time(NULL));

	/* cleanup */
	g_string_free(msg, TRUE);
}

static inline void
stress_close_convs(PurpleAccount *account, const gchar *name) {
	PurpleConversation *conv = NULL;

	conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, name,
												 account);

	if(conv)
		purple_conversation_destroy(conv);
}

static gboolean
stress_event_cb(gpointer data) {
	StressBuddy *sb = (StressBuddy *)data;
	PurpleAccount *account = purple_buddy_get_account(sb->buddy);
	gint event = rand() % nevents;
	const gchar *name = purple_buddy_get_name(sb->buddy);

	/* increment our event counter */
	sb->nevents++;

	event = GPOINTER_TO_INT(g_list_nth_data(events, event));

	purple_debug_info("stress", "firing event %d of %d for %s\n",
					  sb->nevents, sb->maxevents, name);

	stress_close_convs(account, name);

	switch(event) {
		case STRESS_EVENT_SIGN_ON:
		case STRESS_EVENT_BACK:
			purple_prpl_got_user_status(account, name, "available", NULL);
			break;
		case STRESS_EVENT_SIGN_OFF:
			purple_prpl_got_user_status(account, name, "offline", NULL);
			break;
		case STRESS_EVENT_IDLE:
			purple_prpl_got_user_idle(account, name, TRUE, 0);
			break;
		case STRESS_EVENT_UNIDLE:
			purple_prpl_got_user_idle(account, name, FALSE, 0);
			break;
		case STRESS_EVENT_AWAY:
			purple_prpl_got_user_status(account, name, "away", NULL);
			break;
		case STRESS_EVENT_SEND_MESSAGE:
			stress_send_im(account, sb->buddy, name);
			break;
	}

	if(sb->maxevents > 0 && sb->nevents >= sb->maxevents) {
		return FALSE;
	}

	return TRUE;
}

/******************************************************************************
 * PRPL Stuff
 *****************************************************************************/
static const gchar *
stress_list_icon(PurpleAccount *account, PurpleBuddy *b) {
	return NULL;
}

#define add_event(setting, e1, e2) G_STMT_START { \
	if(purple_account_get_bool(account, (setting), TRUE)) { \
		events = g_list_prepend(events, GINT_TO_POINTER((e1))); \
		if(e2 > -1) \
			events = g_list_prepend(events, GINT_TO_POINTER((e2))); \
	} \
} G_STMT_END

static void
stress_login(PurpleAccount *account) {
	PurpleConnection *pc = NULL;
	PurpleGroup *g = NULL;
	gint n_buddies = 0, i = 0, interval = 0, maxevents = 0;

	/* build our possible events from the account settings */
	add_event("trigger_signon", STRESS_EVENT_SIGN_ON, STRESS_EVENT_SIGN_OFF);
	add_event("trigger_idle", STRESS_EVENT_IDLE, STRESS_EVENT_UNIDLE);
	add_event("trigger_away", STRESS_EVENT_AWAY, STRESS_EVENT_BACK);
	add_event("trigger_typing", STRESS_EVENT_TYPING, STRESS_EVENT_STOPPED_TYPING);
	add_event("send_messages", STRESS_EVENT_SEND_MESSAGE, -1);

	nevents = g_list_length(events);

	/* get our connection and set it as online */
	pc = purple_account_get_connection(account);
	purple_connection_set_state(pc, PURPLE_CONNECTED);

	/* grab the account settings we need for buddies */
	n_buddies = purple_account_get_int(account, "nbuddies", 50);
	maxevents = purple_account_get_int(account, "maxevents", 100);
	interval = (guint)purple_account_get_int(account, "interval", 500);
	message_min = purple_account_get_int(account, "message_min", 16);
	message_max = purple_account_get_int(account, "message_max", 128);

	g = purple_group_new("prpl-stress");

	for(i = 0; i < n_buddies; i++) {
		PurpleBuddy *b = NULL;
		StressBuddy *sb = NULL;
		gchar *name = NULL;

		/* create the buddy and it's name */
		name = g_strdup_printf("stress-%04x", i);
		b = purple_buddy_new(account, name, NULL);
		g_free(name);

		/* add our data to the buddy */
		sb = g_new0(StressBuddy, 1);
		sb->buddy = b;
		sb->maxevents = maxevents;
		purple_buddy_set_protocol_data(b, sb);

		/* add the buddy to our list and the purple blist */
		buddies = g_list_prepend(buddies, sb);
		purple_blist_add_buddy(b, NULL, g, NULL);

		/* add our event timer to the buddy */
		sb->timer_id = g_timeout_add(interval, stress_event_cb, sb);
	}
}

static void
stress_close(PurpleConnection *pc) {
	GList *l = NULL;
	PurpleGroup *g = NULL;

	for(l = buddies; l; l = l->next) {
		StressBuddy *sb = l->data;
		purple_blist_remove_buddy(sb->buddy);
	}

	g_list_free(buddies);

	g = purple_find_group("prpl-stress");
	purple_blist_remove_group(g);

	buddies = NULL;
}

static void
stress_buddy_free(PurpleBuddy *buddy) {
	StressBuddy *sb = STRESS_BUDDY(buddy);

	if(!sb)
		return;

	if(sb->timer_id > 0)
		g_source_remove(sb->timer_id);

	g_free(sb);
}

static GList *
stress_status_types(PurpleAccount *account) {
	GList *types = NULL;
	PurpleStatusType *type = NULL;

	g_return_val_if_fail(account != NULL, NULL);

	type = purple_status_type_new_full(PURPLE_STATUS_AVAILABLE,
									   NULL, NULL, TRUE, TRUE, FALSE);
	types = g_list_prepend(types, type);

	type = purple_status_type_new_full(PURPLE_STATUS_OFFLINE,
									   NULL, NULL, TRUE, TRUE, FALSE);
	types = g_list_prepend(types, type);

	type = purple_status_type_new_full(PURPLE_STATUS_AWAY,
									   NULL, NULL, TRUE, TRUE, FALSE);
	types = g_list_prepend(types, type);

	return types;
}

static PurplePluginProtocolInfo prpl_info = {
	OPT_PROTO_NO_PASSWORD,
	NULL,
	NULL,
	NO_BUDDY_ICONS,
	stress_list_icon,
	NULL,
	NULL,
	NULL,
	stress_status_types,
	NULL,
	NULL,
	NULL,
	stress_login,
	stress_close,
	NULL, /* stress_send_im, */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL, /* stress_add_buddies, */
	NULL,
	NULL, /* stress_remove_buddies, */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	stress_buddy_free,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	0,
	NULL,
	NULL,
	NULL,
};

/******************************************************************************
 * Plugin Stuff
 *****************************************************************************/
static gboolean
plugin_load(PurplePlugin *plugin) {
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin) {
	return TRUE;
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_PROTOCOL,
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,

	"core-plugin_pack-stress",
	NULL,
	PP_VERSION,
	NULL,
	NULL,
	"Gary Kramlich <grim@reaperworld.com>",
	PP_WEBSITE,

	plugin_load,
	plugin_unload,
	NULL,

	NULL,
	&prpl_info,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

#define add_option(type, label, name, def) G_STMT_START { \
	option = purple_account_option_##type##_new((label), (name), (def)); \
	prpl_info.protocol_options = g_list_prepend(prpl_info.protocol_options, (option)); \
} G_STMT_END

static void
init_plugin(PurplePlugin *plugin) {
	PurpleAccountOption *option = NULL;

#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	/* seed the randome number generator */
	srand(time(NULL));

	info.name = _("Stress");
	info.summary = _("A PRPL to stress libpurple");
	info.description = info.summary;

	add_option(int, _("Buddies to stress with"), "nbuddies", 50);
	add_option(int, _("Event interval, in milliseconds"), "interval", 500);
	add_option(int, _("Max events per buddy"), "maxevents", 100);
	add_option(bool, _("Trigger signoff/signoff"), "trigger_signon", TRUE);
	add_option(bool, _("Trigger idle/unidle"), "trigger_idle", TRUE);
	add_option(bool, _("Trigger away/back"), "trigger_away", TRUE);
	add_option(bool, _("Trigger typing/stopped typing"), "trigger_typing", TRUE);
	add_option(bool, _("Send messages"), "send_messages", TRUE);
	add_option(int, _("Minimum message length"), "message_min", 16);
	add_option(int, _("Maxium message length"), "message_max", 128);

	prpl_info.protocol_options = g_list_reverse(prpl_info.protocol_options);
}

PURPLE_INIT_PLUGIN(stress, init_plugin, info)

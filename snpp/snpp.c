/*
 * gaim-snpp Protocol Plugin
 *
 * Copyright (C) 2004-2008 Don Seiler <don@seiler.us>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 *
 */

/* If you can't figure out what this line is for, DON'T TOUCH IT. */
#include "../common/pp_internal.h"

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>

#ifndef _WIN32
# include <sys/socket.h>
#else
# include <libc_interface.h>
#endif

#include <sys/types.h>
#include <unistd.h>

#include <plugin.h>
#include <accountopt.h>
#include <prpl.h>
#include <conversation.h>
#include <notify.h>
#include <debug.h>
#include <blist.h>
#include <util.h>

#define SNPP_DEFAULT_SERVER "localhost"
#define SNPP_DEFAULT_PORT 444

#define SNPP_INITIAL_BUFSIZE 1024

enum state {
	CONN,
	PAGE,
	MESS,
	SEND,
	QUIT,
	LOGI
};

struct snpp_data {
	PurpleAccount *account;
	int fd;

	struct snpp_page *current_page;
};

struct snpp_page {
	char *pager;
	char *message;
	int state;
};

static PurplePlugin *_snpp_plugin = NULL;

static struct snpp_page *snpp_page_new()
{
	struct snpp_page *sp;

	purple_debug_info("snpp", "snpp_page_new\n");
	sp = g_new0(struct snpp_page, 1);
	sp->state = CONN;
	return sp;
};

static void snpp_page_destroy(struct snpp_page *sp)
{
	purple_debug_info("snpp", "snpp_page_destroy\n");

	if (sp->pager != NULL)
		g_free(sp->pager);

	if (sp->message != NULL)
		g_free(sp->message);

	g_free(sp);
	sp = NULL;
};

static void snpp_send(gint fd, const char *buf)
{
	purple_debug_info("snpp", "snpp_send\n");

	purple_debug_info("snpp", "snpp_send: sending %s\n", buf);
	if (!write(fd,buf,strlen(buf))) {
		purple_debug_warning("snpp", "snpp_send: Error sending message\n");
	}
};

static void snpp_reset(PurpleConnection *gc, struct snpp_data *sd)
{
	purple_debug_info("snpp", "snpp_reset\n");
	if (gc->inpa)
		purple_input_remove(gc->inpa);

	close(sd->fd);

	if (sd->current_page != NULL)
		snpp_page_destroy(sd->current_page);
}

static int snpp_cmd_logi(struct snpp_data *sd)
{
	char command[SNPP_INITIAL_BUFSIZE];
	const char *password;

	purple_debug_info("snpp", "snpp_cmd_logi\n");

	if ((password = purple_account_get_password(sd->account)) == NULL)
		password = "";

	/* If LOGI is unsupported, this should return 500 */
	sd->current_page->state = LOGI;
	g_snprintf(command, sizeof(command), "LOGI %s %s\n",
			purple_account_get_username(sd->account),
			password);

	snpp_send(sd->fd, command);

	return 0;
}

static int snpp_cmd_page(struct snpp_data *sd)
{
	char command[SNPP_INITIAL_BUFSIZE];

	purple_debug_info("snpp", "snpp_cmd_page\n");

	sd->current_page->state = PAGE;
	g_snprintf(command, sizeof(command), "PAGE %s\n", sd->current_page->pager);
	snpp_send(sd->fd, command);

	return 0;
};

static int snpp_cmd_mess(struct snpp_data *sd)
{
	char command[SNPP_INITIAL_BUFSIZE];

	purple_debug_info("snpp", "snpp_cmd_mess\n");

	sd->current_page->state = MESS;
	g_snprintf(command, sizeof(command), "MESS %s\n", sd->current_page->message);
	snpp_send(sd->fd, command);

	return 0;
};

static int snpp_cmd_send(struct snpp_data *sd)
{
	char command[SNPP_INITIAL_BUFSIZE];

	purple_debug_info("snpp", "snpp_cmd_send\n");

	sd->current_page->state = SEND;
	g_snprintf(command, sizeof(command), "SEND\n");
	snpp_send(sd->fd, command);

	return 0;
};

static int snpp_cmd_quit(struct snpp_data *sd)
{
	char command[SNPP_INITIAL_BUFSIZE];

	purple_debug_info("snpp", "snpp_cmd_quit\n");

	sd->current_page->state = QUIT;
	g_snprintf(command, sizeof(command), "QUIT\n");
	snpp_send(sd->fd, command);

	return 0;
};

static void snpp_callback(gpointer data, gint source, PurpleInputCondition cond)
{
	PurpleConnection *gc;
	struct snpp_data *sd;
	int len;
	char buf[SNPP_INITIAL_BUFSIZE];
	char *retcode = NULL;
	PurpleConversation *conv;

	purple_debug_info("snpp", "snpp_callback\n");

	gc = data;
	sd = gc->proto_data;

	if ((len = read(sd->fd, buf, SNPP_INITIAL_BUFSIZE - 1)) < 0) {
		purple_debug_warning("snpp", "snpp_callback: Read error\n");
		/* purple_connection_error(gc, _("Read error")); */
		return;
	} else if (len == 0) {
		/* Remote closed the connection, probably */
		return;
	}

	buf[len] = '\0';
	purple_debug_info("snpp", "snpp_callback: Recv: %s\n", buf);
	retcode = g_strndup(buf,3);

	if (sd->current_page != NULL) {
		purple_debug_info("snpp", "snpp_callback: Current page found.\n");
		/*
		 * Evaluate state and return code and call appropriate function
		 * to faciliate processing of pages.
		 */

		switch (sd->current_page->state) {
		case CONN:
			purple_debug_info("snpp", "snpp_callback: State is CONN, return code was %s\n", retcode);
			if (!g_ascii_strcasecmp(retcode,"220"))
				snpp_cmd_logi(sd);
			else {
				purple_notify_error(gc, NULL, buf, NULL);
				snpp_reset(gc, sd);
			}
			break;

		case LOGI:
			purple_debug_info("snpp", "snpp_callback: State is LOGI, return code was %s\n", retcode);
			/* If LOGI is unsupported, server should return 500
			   XXX 230 is crutch for HylaFAX breaking the protocol */
			if (!g_ascii_strcasecmp(retcode,"250")
					|| !g_ascii_strcasecmp(retcode, "500")
					|| !g_ascii_strcasecmp(retcode, "230"))
				snpp_cmd_page(sd);
			else {
				purple_notify_error(gc, NULL, buf, NULL);
				snpp_reset(gc, sd);
			}
			break;

		case PAGE:
			purple_debug_info("snpp", "snpp_callback: State is PAGE, return code was %s\n", retcode);
			if (!g_ascii_strcasecmp(retcode,"250"))
				snpp_cmd_mess(sd);
			else {
				purple_notify_error(gc, NULL, buf, NULL);
				snpp_reset(gc, sd);
			}
			break;

		case MESS:
			purple_debug_info("snpp", "snpp_callback: State is MESS, return code was %s\n", retcode);
			if (!g_ascii_strcasecmp(retcode,"250"))
				snpp_cmd_send(sd);
			else {
				purple_notify_error(gc, NULL, buf, NULL);
				snpp_reset(gc, sd);
			}
			break;

		case SEND:
			purple_debug_info("snpp", "snpp_callback: State is SEND, return code was %s\n", retcode);
			if (!g_ascii_strcasecmp(retcode,"250")
				|| !g_ascii_strcasecmp(retcode,"860")
				|| !g_ascii_strcasecmp(retcode,"960")) {
				/* Print status message (buf) to window */
				if ((conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, sd->current_page->pager, sd->account))) {
					purple_conversation_write(conv, NULL, buf, PURPLE_MESSAGE_SYSTEM, time(NULL));
				}
				snpp_cmd_quit(sd);
			} else {
				purple_notify_error(gc, NULL, buf, NULL);
				snpp_reset(gc, sd);
			}
			break;

		case QUIT:
			/* Not sure if we should ever get here */
			purple_debug_info("snpp", "snpp_callback: State is QUIT, return code was %s\n", retcode);

			if (g_ascii_strcasecmp(retcode,"221"))
				purple_debug_info("snpp", "snpp_callback: Return code of 221 expected, not received\n");

			snpp_reset(gc, sd);
			break;

		default:
			purple_debug_info("snpp", "snpp_callback: current_page in unknown state\n");
			purple_notify_error(gc, NULL, buf, NULL);
		}

	} else {
		purple_debug_info("snpp", "snpp_callback: No current page to process\n");
	}

	g_free(retcode);
};

static void snpp_connect_cb(gpointer data, gint source, const gchar *error_message)
{
	PurpleConnection *gc;
	struct snpp_data *sd;
	GList *connections;

	purple_debug_info("snpp", "snpp_connect_cb\n");

	gc = data;
	sd = gc->proto_data;
	connections = purple_connections_get_all();

	if (source < 0)
		return;

	if (!g_list_find(connections, gc)) {
		close(source);
		return;
	}

	sd->fd = source;

	gc->inpa = purple_input_add(sd->fd, PURPLE_INPUT_READ, snpp_callback, gc);
}

static void snpp_connect(PurpleConnection *gc)
{
	PurpleProxyConnectData *ppcd = NULL;

	purple_debug_info("snpp", "snpp_connect\n");

	ppcd =	purple_proxy_connect(gc->account, gc->account,
			purple_account_get_string(gc->account, "server", SNPP_DEFAULT_SERVER),
			purple_account_get_int(gc->account, "port", SNPP_DEFAULT_PORT),
			snpp_connect_cb,
			gc);

	if (!ppcd || !gc->account->gc) {
		purple_connection_error(gc, _("Couldn't connect to SNPP server"));
		return;
	}
};


static int snpp_process(PurpleConnection *gc, struct snpp_data *sd)
{
	purple_debug_info("snpp", "snpp_process\n");

	if (sd->current_page->message && (strlen(sd->current_page->message) > 0)) {
		/* Just completed proxy_connect, ready to send data to server */
		purple_debug_info("snpp", "snpp_page: Sending SNPP Request:\n\n%s\n\n", sd->current_page->message);

		/* Get ball rolling, snpp_callback will take over */
		snpp_connect(gc);
	}

	return 1;
};




static int snpp_send_im(PurpleConnection *gc,
						const char *who,
						const char *what,
						PurpleMessageFlags flags)
{
	struct snpp_data *sd;
	struct snpp_page *sp;

	purple_debug_info("snpp", "snpp_send_im\n");
	sd = gc->proto_data;
	sp = snpp_page_new();

	sp->pager = g_strdup(who);
	sp->message = g_strdup(what);

	sd->current_page = sp;
	snpp_process(gc, sd);

	return 1;
};


static const char *snpp_icon(PurpleAccount *a, PurpleBuddy *b)
{
	/* purple_debug_info("snpp", "snpp_icon\n"); */
	return "snpp";
}

static void fake_buddy_signons(PurpleAccount *account)
{
	PurpleBlistNode *node = purple_get_blist()->root;

	while (node != NULL)
	{
		if (PURPLE_BLIST_NODE_IS_BUDDY(node))
		{
			PurpleBuddy *buddy = (PurpleBuddy *)node;
			if (buddy->account == account)
				purple_prpl_got_user_status(account, buddy->name, "available", NULL);
		}

		node = purple_blist_node_next(node, FALSE);
	}

}

static void snpp_login(PurpleAccount *account)
{
	PurpleConnection *gc;
	struct snpp_data *sd;

	purple_debug_info("snpp", "snpp_login\n");

	gc = purple_account_get_connection(account);
	gc->proto_data = sd = g_new0(struct snpp_data, 1);
	sd->account = account;

	purple_connection_set_state(gc, PURPLE_CONNECTED);

	fake_buddy_signons(account);
};


static void snpp_close(PurpleConnection *gc)
{
	struct snpp_data *sd;
	purple_debug_info("snpp", "snpp_close\n");

	sd = gc->proto_data;

	if (sd == NULL)
		return;

	snpp_reset(gc, sd);
	g_free(sd);
};

static void snpp_add_buddy(PurpleConnection *gc, PurpleBuddy *b, PurpleGroup *group)
{
	purple_debug_info("snpp", "snpp_add_buddy\n");
	purple_prpl_got_user_status(purple_connection_get_account(gc), b->name, "available", NULL);
};

static void snpp_remove_buddy(PurpleConnection *gc, PurpleBuddy *b, PurpleGroup *group)
{
	purple_debug_info("snpp", "snpp_remove_buddy\n");
};

static GList *snpp_status_types(PurpleAccount *account)
{
	PurpleStatusType *status;
	GList *types = NULL;

	status = purple_status_type_new_full(PURPLE_STATUS_OFFLINE,
									   NULL, NULL, FALSE, TRUE, FALSE);
	types = g_list_append(types, status);

	status = purple_status_type_new_full(PURPLE_STATUS_AVAILABLE,
									   NULL, NULL, FALSE, TRUE, FALSE);
	types = g_list_append(types, status);

	return types;
}

static PurplePluginProtocolInfo prpl_info =
{
	OPT_PROTO_PASSWORD_OPTIONAL,	/* options */
	NULL,							/* user_splits */
	NULL,							/* protocol_options */
	NO_BUDDY_ICONS,					/* icon_spec */
	snpp_icon,						/* list_icon */
	NULL,							/* list_emblems */
	NULL,							/* status_text */
	NULL,							/* tooltip_text */
	snpp_status_types,				/* status_types */
	NULL,							/* blist_node_menu */
	NULL,							/* chat_info */
	NULL,							/* chat_info_defaults */
	snpp_login,						/* login */
	snpp_close,						/* close */
	snpp_send_im,					/* send_im */
	NULL,							/* set_info */
	NULL,							/* send_typing */
	NULL,							/* get_info */
	NULL,							/* set_away */
	NULL,							/* set_idle */
	NULL,							/* change_password */
	snpp_add_buddy,					/* add_buddy */
	NULL,							/* add_buddies */
	snpp_remove_buddy,				/* remove_buddy */
	NULL,							/* remove_buddies */
	NULL,							/* add_permit */
	NULL,							/* add_deny */
	NULL,							/* rem_permit */
	NULL,							/* rem_deny */
	NULL,							/* set_permit_deny */
	NULL,							/* warn */
	NULL,							/* join_chat */
	NULL,							/* reject_chat */
	NULL,							/* chat_invite */
	NULL,							/* chat_leave */
	NULL,							/* chat_whisper */
	NULL,							/* chat_send */
	NULL,							/* keepalive */
	NULL,							/* register_user */
	NULL,							/* get_cb_info */
	NULL,							/* get_cb_away */
	NULL,							/* alias_buddy */
	NULL,							/* group_buddy */
	NULL,							/* rename_group */
	NULL,							/* buddy_free */
	NULL,							/* convo_closed */
	NULL,							/* normalize */
	NULL,							/* set_buddy_icon */
	NULL,							/* remove_group */
	NULL,							/* get_cb_real_name */
	NULL,							/* set_chat_topic */
	NULL,							/* find_blist_chat */
	NULL,							/* roomlist_get_list */
	NULL,							/* roomlist_cancel */
	NULL,							/* roomlist_expand_catagory */
	NULL,							/* can_receive_file */
	NULL,							/* send_file */
	NULL,							/* new_xfer */
	NULL,							/* offline_message */
	NULL,							/* whiteboard_prpl_ops */
	NULL,							/* send_raw */
	NULL,							/* roomlist_room_serialize */
	NULL,							/* unregister_user */
	NULL,							/* send_attention */
	NULL,							/* get_attention_types */
	NULL							/* reserved 4 */
};


static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,			/* magic */
	PURPLE_MAJOR_VERSION,			/* purple major */
	PURPLE_MINOR_VERSION,			/* purple minor */
	PURPLE_PLUGIN_PROTOCOL,			/* type */
	NULL,							/* ui_requirement */
	0,								/* flags */
	NULL,							/* dependencies */
	PURPLE_PRIORITY_DEFAULT,		/* priority */
	"prpl-snpp",					/* id */
	NULL,							/* name */
	PP_VERSION,						/* version */
	NULL,							/* summary */
	NULL,							/* description */
	"Don Seiler <don@seiler.us>",	/* author */
	PP_WEBSITE,						/* homepage */
	NULL,							/* load */
	NULL,							/* unload */
	NULL,							/* destroy */
	NULL,							/* ui_info */
	&prpl_info,						/* extra_info */
	NULL,							/* prefs_info */
	NULL,							/* actions */
	NULL,							/* reserved */
	NULL,							/* reserved */
	NULL,							/* reserved */
	NULL							/* reserved */
};


static void _init_plugin(PurplePlugin *plugin)
{
	PurpleAccountOption *option;
	option = purple_account_option_string_new(_("Server"), "server", SNPP_DEFAULT_SERVER);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_int_new(_("Port"), "port", SNPP_DEFAULT_PORT);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	_snpp_plugin = plugin;

	info.name = _("SNPP");
	info.summary = _("SNPP Plugin");
	info.description =
		_("Allows libpurple to send messages over the Simple Network Paging Protocol (SNPP).");
};

PURPLE_INIT_PLUGIN(snpp, _init_plugin, info);

/*
 * IRC Helper Plugin for libpurple
 *
 * Copyright (C) 2005-2007, Richard Laager <rlaager@pidgin.im>
 * Copyright (C) 2004-2005, Mathias Hasselmann <mathias@taschenorakel.de>
 * Copyright (C) 2005, Daniel Beardsmore <uilleann@users.sf.net>
 * Copyright (C) 2005, Björn Nilsson <BNI on irc.freenode.net>
 * Copyright (C) 2005, Anthony Sofocleous <itchysoft_ant@users.sf.net>
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

#include <string.h>

#include <account.h>
#include <accountopt.h>
#include <cmds.h>
#include <connection.h>
#include <conversation.h>
#include <debug.h>
#include <notify.h>
#include <plugin.h>
#include <pluginpref.h>
#include <prefs.h>
#include <util.h>

#define PLUGIN_STATIC_NAME "irchelper"
#define PLUGIN_ID "core-rlaager-" PLUGIN_STATIC_NAME

#define PLUGIN_AUTHOR      "Richard Laager <rlaager@guifications.org>"


/*****************************************************************************
 * Constants                                                                 *
 *****************************************************************************/

#define IRC_PLUGIN_ID "prpl-irc"

/* Copied from SECS_BEFORE_RESENDING_AUTORESPONSE in src/server.c */
#define AUTO_RESPONSE_INTERVAL 600

#define DOMAIN_SUFFIX_FREENODE ".freenode.net"
#define DOMAIN_SUFFIX_GAMESURGE ".gamesurge.net"
#define DOMAIN_SUFFIX_THUNDERCITY ".thundercity.org"
#define DOMAIN_SUFFIX_DALNET ".dal.net"
#define DOMAIN_SUFFIX_JEUX ".jeux.fr"
#define DOMAIN_SUFFIX_QUAKENET ".quakenet.org"
#define DOMAIN_SUFFIX_UNDERNET ".undernet.org"

#define MESSAGE_CHANSERV_ACCESS_LIST_ADD "You have been added to the access list for "
/* Omit the first character of the portion after the channel name. */
#define MESSAGE_CHANSERV_ACCESS_LIST_ADD_WITH_LEVEL "with level ["
#define MESSAGE_CHANSERV_ACCESS_LIST_DEL "You have been deleted from the access list for ["
#define MESSAGE_CHANSERV_NO_OP_ACCESS "You do not have channel operator access to"
#define MESSAGE_FRENCH_ADVERTISEMENT_START "<B>Avertissement</B> : Le pseudo <B>"
#define MESSAGE_FRENCH_ADVERTISEMENT_END "&lt;votre pass&gt;"
#define MESSAGE_FRENCH_LOGIN "Login <B>r?ussi</B>"
#define MESSAGE_FRENCH_MAXIMUM_CONNECTION_COUNT "Maximum de connexion"
#define MESSAGE_FRENCH_MOTD "Message du Jour :"
#define MESSAGE_PURPLE_NOTICE_PREFIX "(notice) "
#define MESSAGE_GAMESURGE_AUTHSERV_IDENTIFIED "I recognize you."
#define MESSAGE_GAMESURGE_AUTHSERV_ID_FAILURE "Incorrect password; please try again."
#define MESSAGE_GHOST_KILLED " has been killed"
#define MESSAGE_INVITED " invited "
#define MESSAGE_LOGIN_CONNECTION_COUNT "Highest connection count"
#define MESSAGE_MEMOSERV_NO_NEW_MEMOS "You have no new memos"
#define MESSAGE_MODE_NOTICE_PREFIX "mode (+"
#define MESSAGE_MODE_NOTICE_SUFFIX " ) by "
#define MESSAGE_NICKSERV_CLOAKED " set your hostname to"
#define MESSAGE_NICKSERV_ID_FAILURE "Password Incorrect"
#define MESSAGE_NICKSERV_IDENTIFIED "Password accepted - you are now recognized"
#define MESSAGE_QUAKENET_Q_CRUFT \
	"Remember: NO-ONE from QuakeNet will ever ask for your password.  " \
	"NEVER send your password to ANYONE except Q@CServe.quakenet.org."
#define MESSAGE_QUAKENET_Q_ID_FAILURE \
	"Lastly, When you do recover your password, please choose a NEW PASSWORD, not your old one! " \
	"See the above URL for details."
#define MESSAGE_QUAKENET_Q_IDENTIFIED "AUTH&apos;d successfully."
#define MESSAGE_UNREAL_IRCD_HOSTNAME_FOUND "*** Found your hostname"
#define MESSAGE_UNREAL_IRCD_HOSTNAME_LOOKUP "*** Looking up your hostname..."
#define MESSAGE_UNREAL_IRCD_IDENT_LOOKUP "*** Checking ident..."
#define MESSAGE_UNREAL_IRCD_IDENT_NO_RESPONSE "*** No ident response; username prefixed with ~"
#define MESSAGE_UNREAL_IRCD_PONG_CRUFT \
	"*** If you are having problems connecting due to ping timeouts, please type /quote pong"

/* Generic AuthServ, not currently used for any networks. */
#define NICK_AUTHSERV "AuthServ"

#define NICK_CHANSERV "ChanServ"
#define NICK_FREENODE_CONNECT "freenode-connect"
#define NICK_GAMESURGE_AUTHSERV "AuthServ"
#define NICK_GAMESURGE_AUTHSERV_SERVICE NICK_GAMESURGE_AUTHSERV "@Services.GameSurge.net"
#define NICK_GAMESURGE_GLOBAL "Global"
#define NICK_JEUX_Z "Z"
#define NICK_JEUX_WELCOME "[Welcome]"
#define NICK_MEMOSERV "MemoServ"
#define NICK_NICKSERV "NickServ"
#define NICK_DALNET_AUTHSERV_SERVICE NICK_NICKSERV "@services.dal.net"
#define NICK_QUAKENET_L "L"
#define NICK_QUAKENET_Q "Q"
#define NICK_QUAKENET_Q_SERVICE NICK_QUAKENET_Q "@CServe.quakenet.org"
#define NICK_UNDERNET_X "x@channels.undernet.org"

/* The %c exists so we can tell if a match occurred. */
#define PATTERN_WEIRD_LOGIN_CRUFT "o%c %*u ca %*u(%*u) ft %*u(%*u)"

#define TIMEOUT_IDENTIFY 4000
#define TIMEOUT_KILLING_GHOST 4000


typedef enum {
	IRC_NONE                   = 0x0000,
	IRC_KILLING_GHOST          = 0x0001,
	IRC_WILL_ID                = 0x0002,
	IRC_DID_ID                 = 0x0004,
	IRC_ID_FAILED              = 0x0008,

	IRC_NETWORK_TYPE_UNKNOWN   = 0x0010,
	IRC_NETWORK_TYPE_GAMESURGE = 0x0020,
	IRC_NETWORK_TYPE_NICKSERV  = 0x0040,
	IRC_NETWORK_TYPE_QUAKENET  = 0x0080,
	IRC_NETWORK_TYPE_JEUX      = 0x0100,
	IRC_NETWORK_TYPE_UNDERNET  = 0x0200,
	IRC_NETWORK_TYPE_THUNDERCITY = 0x0400,
	IRC_NETWORK_TYPE_DALNET    = 0x0800,
} IRCHelperStateFlags;

struct proto_stuff
{
	gpointer *proto_data;
	PurpleAccount *account;
};

GHashTable *states;

/*****************************************************************************
 * Prototypes                                                                *
 *****************************************************************************/

static gboolean plugin_load(PurplePlugin *plugin);
static gboolean plugin_unload(PurplePlugin *plugin);


/*****************************************************************************
 * Plugin Info                                                               *
 *****************************************************************************/

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	0,
	PURPLE_PLUGIN_STANDARD,          /**< type           */
	NULL,                            /**< ui_requirement */
	0,                               /**< flags          */
	NULL,                            /**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,         /**< priority       */
	PLUGIN_ID,                       /**< id             */
	NULL,                            /**< name           */
	PP_VERSION,                      /**< version        */
	NULL,                            /**< summary        */
	NULL,                            /**< description    */
	PLUGIN_AUTHOR,	                 /**< author         */
	PP_WEBSITE,                      /**< homepage       */
	plugin_load,                     /**< load           */
	plugin_unload,                   /**< unload         */
	NULL,                            /**< destroy        */

	NULL,                            /**< ui_info        */
	NULL,                            /**< extra_info     */
	NULL,                            /**< prefs_info     */
	NULL,                            /**< actions        */
	NULL,                            /**< reserved 1     */
	NULL,                            /**< reserved 2     */
	NULL,                            /**< reserved 3     */
	NULL                             /**< reserved 4     */
};

/* XXX: This is a dirty hack. It's better than what I was doing before, though. */
static PurpleConversation *get_conversation(PurpleAccount *account)
{
	PurpleConversation *conv;

	conv = g_new0(PurpleConversation, 1);
	conv->type = PURPLE_CONV_TYPE_IM;
	purple_conversation_set_account(conv, account);

	return conv;
}

static IRCHelperStateFlags get_connection_type(PurpleConnection *connection)
{
	PurpleAccount *account;
	const gchar *protocol;
	gchar *username;
	IRCHelperStateFlags type = IRC_NETWORK_TYPE_UNKNOWN;

	g_return_val_if_fail(NULL != connection, IRC_NETWORK_TYPE_UNKNOWN);

	account = purple_connection_get_account(connection);

	protocol = purple_account_get_protocol_id(account);
	g_return_val_if_fail(g_str_equal(protocol, IRC_PLUGIN_ID), IRC_NETWORK_TYPE_UNKNOWN);

	username = g_utf8_strdown(purple_account_get_username(account), -1);

	if (g_str_has_suffix(username, DOMAIN_SUFFIX_GAMESURGE))
		type = IRC_NETWORK_TYPE_GAMESURGE;
	else if (g_str_has_suffix(username, DOMAIN_SUFFIX_THUNDERCITY))
		type = IRC_NETWORK_TYPE_THUNDERCITY;
	else if (g_str_has_suffix(username, DOMAIN_SUFFIX_DALNET))
		type = IRC_NETWORK_TYPE_DALNET;
	else if (g_str_has_suffix(username, DOMAIN_SUFFIX_QUAKENET))
		type = IRC_NETWORK_TYPE_QUAKENET;
	else if (g_str_has_suffix(username, DOMAIN_SUFFIX_JEUX))
		type = IRC_NETWORK_TYPE_JEUX;
	else if (g_str_has_suffix(username, DOMAIN_SUFFIX_UNDERNET))
		type = IRC_NETWORK_TYPE_UNDERNET;

	g_free(username);
	return type;
}

static gboolean auth_timeout(gpointer proto_data)
{
	IRCHelperStateFlags state;

	state = GPOINTER_TO_INT(g_hash_table_lookup(states, proto_data));
	if (state & IRC_WILL_ID)
	{
		purple_debug_info(PLUGIN_STATIC_NAME, "Authentication failed: timeout expired\n");
		g_hash_table_insert(states, proto_data,
		                    GINT_TO_POINTER((state & ~IRC_WILL_ID) | IRC_DID_ID));
	}

	return FALSE;
}


/*****************************************************************************
 * AuthServ Helper Functions                                                 *
 *****************************************************************************/

static void authserv_identify(const char *command, PurpleConnection *connection, IRCHelperStateFlags state)
{
	PurpleAccount *account;
	gchar **userparts = NULL;
	const gchar *username;
	const gchar *password;

	g_return_if_fail(NULL != connection);

	account = purple_connection_get_account(connection);

	username = purple_account_get_string(account, PLUGIN_ID "_authname", "");
	if (NULL == username || '\0' == *username)
	{
		userparts = g_strsplit(purple_account_get_username(account), "@", 2);
		username = userparts[0];
	}

	password = purple_account_get_string(account, PLUGIN_ID "_nickpassword", "");

	if (NULL != username && '\0' != *username &&
	    NULL != password && '\0' != *password)
	{
		const gchar *authserv = NICK_AUTHSERV;
		gchar *authentication = g_strconcat(command, " ", username, " ", password, NULL);

		purple_debug_misc(PLUGIN_STATIC_NAME, "Sending authentication: %s\n", authentication);

		g_hash_table_insert(states,
		                    connection->proto_data,
		                    GINT_TO_POINTER(state | IRC_WILL_ID));

		if (state & IRC_NETWORK_TYPE_GAMESURGE)
			authserv = NICK_GAMESURGE_AUTHSERV_SERVICE;
		else if (state & IRC_NETWORK_TYPE_DALNET)
			authserv = NICK_DALNET_AUTHSERV_SERVICE;
		else if (state & IRC_NETWORK_TYPE_QUAKENET)
			authserv = NICK_QUAKENET_Q_SERVICE;
		else if (state & IRC_NETWORK_TYPE_UNDERNET)
			authserv = NICK_UNDERNET_X;

		serv_send_im(connection, authserv, authentication, 0);

		/* Register a timeout... If we don't get the expected response from AuthServ,
		 * we need to stop suppressing messages from it at some point or the user
		 * could be very confused.
		 */
		purple_timeout_add(TIMEOUT_IDENTIFY, (GSourceFunc)auth_timeout,
		                 (gpointer)connection->proto_data);
	}

	g_strfreev(userparts);
}


/*****************************************************************************
 * Operator Helper Functions                                                 *
 *****************************************************************************/

static void jeux_identify(PurpleConnection *connection, IRCHelperStateFlags state)
{
	PurpleAccount *account;
	gchar **userparts;
	const gchar *username;
	const gchar *password;

	g_return_if_fail(NULL != connection);

	account = purple_connection_get_account(connection);

	userparts = g_strsplit(purple_account_get_username(account), "@", 2);
	username = userparts[0];
	password = purple_account_get_string(account, PLUGIN_ID "_nickpassword", "");

	if (NULL != username && '\0' != *username &&
	    NULL != password && '\0' != *password)
	{
		gchar *authentication = g_strdup_printf("quote %s login %s %s", NICK_JEUX_Z, username, password);
		PurpleConversation *conv = get_conversation(account);
		gchar *error;

		purple_debug_misc(PLUGIN_STATIC_NAME, "Sending authentication: %s\n", authentication);

		g_hash_table_insert(states,
		                    connection->proto_data,
		                    GINT_TO_POINTER(state | IRC_WILL_ID));

		if (purple_cmd_do_command(conv, authentication, authentication, &error) != PURPLE_CMD_STATUS_OK)
		{
			/* TODO: PRINT ERROR MESSAGE */
			g_free(error);
		}
		g_free(conv);

		/* Register a timeout... If we don't get the expected response from AuthServ,
		 * we need to stop suppressing messages from it at some point or the user
		 * could be very confused.
		 */
		purple_timeout_add(TIMEOUT_IDENTIFY, (GSourceFunc)auth_timeout,
		                 (gpointer)connection->proto_data);
	}

	g_strfreev(userparts);
}


/*****************************************************************************
 * Operator Helper Functions                                                 *
 *****************************************************************************/

 static void oper_identify(PurpleAccount *account)
 {
	const char *operpassword;

	operpassword = purple_account_get_string(account, PLUGIN_ID "_operpassword", "");
	if ('\0' != *operpassword)
	{
		PurpleConversation *conv = get_conversation(account);
		PurpleConnection *connection = purple_account_get_connection(account);
		const gchar *name = purple_connection_get_display_name(connection);
		char *command = g_strdup_printf("quote OPER %s %s", name, operpassword);
		gchar *error;

		if (purple_cmd_do_command(conv, command, command, &error) != PURPLE_CMD_STATUS_OK)
		{
			/* TODO: PRINT ERROR MESSAGE */
			g_free(error);
		}

		g_free(command);
		g_free(conv);
	}

 }


/*****************************************************************************
 * NickServ Helper Functions                                                 *
 *****************************************************************************/

static void nickserv_do_identify(char *authentication, gpointer proto_data, PurpleConnection *gc, const char *nickpassword)
{
	PurpleConversation *conv = get_conversation(purple_connection_get_account(gc));
	gchar *error;

	purple_debug_misc(PLUGIN_STATIC_NAME, "Sending authentication: %s\n", authentication);

	if (purple_cmd_do_command(conv, authentication, authentication, &error) != PURPLE_CMD_STATUS_OK)
	{
		/* TODO: PRINT ERROR MESSAGE */
		g_free(error);
	}
	g_free(authentication);
	g_free(conv);

	/* Register a timeout... If we don't get the expected response from NickServ,
	 * we need to stop suppressing messages from it at some point or the user
	 * could be very confused.
	 */
	purple_timeout_add(TIMEOUT_IDENTIFY, (GSourceFunc)auth_timeout,
	                 (gpointer)proto_data);
}

static void nickserv_identify(gpointer proto_data, PurpleConnection *gc, const char *nickpassword)
{
	char *authentication = g_strdup_printf("quote %s IDENTIFY %s", NICK_NICKSERV, nickpassword);
	nickserv_do_identify(authentication, proto_data, gc, nickpassword);
}

static void nickserv_msg_identify(const char *command, gpointer proto_data, PurpleConnection *gc, const char *nickpassword)
{
	char *authentication = g_strdup_printf("quote PRIVMSG %s : %s %s", NICK_NICKSERV, command, nickpassword);
	nickserv_do_identify(authentication, proto_data, gc, nickpassword);
}

static gboolean ghosted_nickname_killed_cb(struct proto_stuff *stuff)
{
	PurpleConnection *gc;
	char **userparts;
	IRCHelperStateFlags state;
	PurpleConversation *conv;
	char *command;
	gchar *error;

	state = GPOINTER_TO_INT(g_hash_table_lookup(states, stuff->proto_data));

	/* We only want this function to act once. Under normal circumstances,
	 * it's going to get called twice: Once when NickServ tells us the ghost
	 * has been killed and once when the timeout expires. Under abnormal
	 * conditions, it will only be called when the timeout expires.
	 */
	if (!(state & IRC_KILLING_GHOST))
	{
		g_free(stuff);
		return FALSE;
	}
	g_hash_table_insert(states, stuff->proto_data,
	                    GINT_TO_POINTER((state & ~IRC_KILLING_GHOST) | IRC_WILL_ID));

	gc = purple_account_get_connection(stuff->account);
	if (NULL == gc)
	{
		g_free(stuff);
		return FALSE;
	}

	userparts = g_strsplit(purple_account_get_username(stuff->account), "@", 2);

	/* Switch back to the normal nickname. */
	conv = get_conversation(stuff->account);

	command = g_strdup_printf("nick %s", userparts[0]);

	if (purple_cmd_do_command(conv, command, command, &error) != PURPLE_CMD_STATUS_OK)
	{
		/* TODO: PRINT ERROR MESSAGE */
		g_free(error);
	}

	g_free(command);
	g_free(conv);

	nickserv_identify(stuff->proto_data, gc, purple_account_get_string(stuff->account,
	                  PLUGIN_ID "_nickpassword", ""));

	g_strfreev(userparts);
	g_free(stuff);

	oper_identify(stuff->account);

	return FALSE;
}


/*****************************************************************************
 * Callbacks                                                                 *
 *****************************************************************************/

static void signed_on_cb(PurpleConnection *connection)
{
	PurpleAccount *account;
	IRCHelperStateFlags state;
	const char *nickpassword;

	g_return_if_fail(NULL != connection);
	g_return_if_fail(NULL != connection->proto_data);

	account = purple_connection_get_account(connection);

	g_return_if_fail(NULL != account);
	if (!g_str_equal(purple_account_get_protocol_id(account), IRC_PLUGIN_ID))
		return;

	state = get_connection_type(connection);

	if (state & IRC_NETWORK_TYPE_GAMESURGE)
	{
		purple_debug_info(PLUGIN_STATIC_NAME, "Connected with GameSurge: %s\n",
		                purple_connection_get_display_name(connection));

		authserv_identify("AUTH", connection, state);
	}
	if (state & IRC_NETWORK_TYPE_DALNET)
	{
		purple_debug_info(PLUGIN_STATIC_NAME, "Connected with DalNet: %s\n",
						purple_connection_get_display_name(connection));

		authserv_identify("IDENTIFY", connection, state);
	}
	else if (state & IRC_NETWORK_TYPE_JEUX)
	{
		purple_debug_info(PLUGIN_STATIC_NAME, "Connected with Jeux.fr: %s\n",
		                purple_connection_get_display_name(connection));

		jeux_identify(connection, state);
	}
	else if (state & IRC_NETWORK_TYPE_QUAKENET)
	{
		purple_debug_info(PLUGIN_STATIC_NAME, "Connected with QuakeNet: %s\n",
		                purple_connection_get_display_name(connection));

		authserv_identify("AUTH", connection, state);
	}
	else if (state & IRC_NETWORK_TYPE_UNDERNET)
	{
		purple_debug_info(PLUGIN_STATIC_NAME, "Connected with UnderNet: %s\n",
		                purple_connection_get_display_name(connection));

		authserv_identify("login ", connection, state);
	}
	else
	{
		nickpassword = purple_account_get_string(account, PLUGIN_ID "_nickpassword", "");
		if ('\0' != *nickpassword)
		{
			char **userparts;

			g_hash_table_insert(states, connection->proto_data,
			                    GINT_TO_POINTER(IRC_NETWORK_TYPE_NICKSERV | IRC_WILL_ID));

			userparts = g_strsplit(purple_account_get_username(account), "@", 2);

			if (purple_account_get_bool(account, PLUGIN_ID "_disconnectghosts", 0) &&
			    strcmp(userparts[0], purple_connection_get_display_name(connection)))
			{
				struct proto_stuff *stuff = g_new0(struct proto_stuff, 1);
				char *command;
				PurpleConversation *conv;
				gchar *error;

				stuff->proto_data = connection->proto_data;
				stuff->account = account;

				/* Disconnect the ghosted connection. */
				command = g_strdup_printf("quote %s GHOST %s %s", NICK_NICKSERV, userparts[0], nickpassword);
				conv = get_conversation(account);

				purple_debug_misc(PLUGIN_STATIC_NAME, "Sending command: %s\n", command);

				if (purple_cmd_do_command(conv, command, command, &error) != PURPLE_CMD_STATUS_OK)
				{
					/* TODO: PRINT ERROR MESSAGE */
					g_free(error);
				}
				g_free(command);
				g_free(conv);

				g_hash_table_insert(states, connection->proto_data,
				                    GINT_TO_POINTER(IRC_NETWORK_TYPE_NICKSERV | IRC_KILLING_GHOST));

				/* We have to wait for the server to disconnect the
				 * other username before we can reclaim it. This
				 * timeout sets an upper bound on the length of time
				 * we'll wait for the ghost to be killed.
				 */
				purple_timeout_add(TIMEOUT_KILLING_GHOST,
				                 (GSourceFunc)ghosted_nickname_killed_cb,
				                 (gpointer)stuff);

				g_strfreev(userparts);
				return;
			}

			g_strfreev(userparts);


			if (state & IRC_NETWORK_TYPE_THUNDERCITY)
				nickserv_msg_identify("AUTH", connection->proto_data, connection, nickpassword);
			else
				nickserv_identify(connection->proto_data, connection, nickpassword);
		}
	}

	oper_identify(account);
}

static void conversation_created_cb(PurpleConversation *conv)
{
	purple_conversation_set_data(conv, PLUGIN_ID "_start_time",
	                           GINT_TO_POINTER((int)time(NULL)));
}

static gboolean writing_chat_msg_cb(PurpleAccount *account, const char *who,
                                  char **message, PurpleConversation *conv,
                                  PurpleMessageFlags flags)
{
	const gchar *name;
	const gchar *topic;

	if (!g_str_equal(purple_account_get_protocol_id(account), IRC_PLUGIN_ID))
		return FALSE;

	if (NULL == *message)
		return FALSE;

	g_return_val_if_fail(purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT, FALSE);

	if (flags & PURPLE_MESSAGE_SYSTEM &&
	    (g_str_has_prefix(*message, MESSAGE_MODE_NOTICE_PREFIX "v ") ||
	     g_str_has_prefix(*message, MESSAGE_MODE_NOTICE_PREFIX "o ")))
	{
		const char *tmp = *message + sizeof(MESSAGE_MODE_NOTICE_PREFIX "X ") - 1;
		const char *name = purple_connection_get_display_name(purple_account_get_connection(account));

		if (g_str_has_prefix(tmp, name) &&
		    g_str_has_prefix(tmp + strlen(name), MESSAGE_MODE_NOTICE_SUFFIX NICK_CHANSERV))
		{
			if (time(NULL) < (time_t)GPOINTER_TO_INT(purple_conversation_get_data(conv, PLUGIN_ID "_start_time")) + 10)
				return TRUE;
		}
	}

	if (flags & PURPLE_MESSAGE_SYSTEM &&
	    (topic = purple_conv_chat_get_topic(PURPLE_CONV_CHAT(conv))) != NULL &&
	    (name = purple_conversation_get_name(conv)) != NULL)
	{
		char *name_escaped = g_markup_escape_text(name, -1);
		char *topic_escaped = g_markup_escape_text(topic, -1);
		char *topic_linkified = purple_markup_linkify(topic_escaped);

		if (strstr(*message, name_escaped) != NULL &&
		    strstr(*message, topic_linkified) != NULL)
		{
			/* We've got a topic notice. */
			PurpleBlistNode *node;

			if ((node = (PurpleBlistNode *)purple_blist_find_chat(account, name)) != NULL)
			{
				const char *last_topic = purple_blist_node_get_string(node, PLUGIN_ID "_topic");

				/* If we saw this the last time we joined, suppress it. */
				if (last_topic != NULL && strcmp(topic, last_topic) == 0)
				{
					g_free(name_escaped);
					g_free(topic_escaped);
					g_free(topic_linkified);

					return TRUE;
				}
				else
					purple_blist_node_set_string(node, PLUGIN_ID "_topic", topic);
			}
		}
		g_free(name_escaped);
		g_free(topic_escaped);
		g_free(topic_linkified);
	}

	return FALSE;
}

static GSList *auto_responses = NULL;

struct auto_response {
	PurpleConnection *gc;
	char *name;
	time_t received;
	char *message;
};

static gboolean
expire_auto_responses(gpointer data)
{
	GSList *tmp, *cur;
	struct auto_response *ar;

	tmp = auto_responses;

	while (tmp) {
		cur = tmp;
		tmp = tmp->next;
		ar = (struct auto_response *)cur->data;

		if ((time(NULL) - ar->received) > AUTO_RESPONSE_INTERVAL) {
			auto_responses = g_slist_remove(auto_responses, ar);

			g_free(ar->message);
			g_free(ar);
		}
	}

	return FALSE; /* do not run again */
}

static struct auto_response *
get_auto_response(PurpleConnection *gc, const char *name)
{
	GSList *tmp;
	struct auto_response *ar;

	purple_timeout_add((AUTO_RESPONSE_INTERVAL + 1) * 1000, expire_auto_responses, NULL);

	tmp = auto_responses;

	while (tmp) {
		ar = (struct auto_response *)tmp->data;

		if (gc == ar->gc && !strncmp(name, ar->name, sizeof(ar->name)))
			return ar;

		tmp = tmp->next;
	}

	ar = (struct auto_response *)g_new0(struct auto_response, 1);
	ar->name = g_strdup(name);
	ar->gc = gc;
	ar->received = 0;
	auto_responses = g_slist_prepend(auto_responses, ar);

	return ar;
}

static gboolean receiving_im_msg_cb(PurpleAccount *account, gchar **sender, gchar **buffer,
                                    PurpleConversation *conv,
                                    gint *flags, gpointer data)
{
	gchar *msg;
	gchar *nick;
	PurpleConversation *chat;
	PurpleConnection *connection;
	IRCHelperStateFlags state;
	gchar *invite_prefix;

	if (!g_str_equal(purple_account_get_protocol_id(account), IRC_PLUGIN_ID))
		return FALSE;

	msg = *buffer;
	nick = *sender;

	connection = purple_account_get_connection(account);
	g_return_val_if_fail(NULL != connection, FALSE);
	state = GPOINTER_TO_INT(g_hash_table_lookup(states, connection->proto_data));

	/* SUPPRESS EXTRA AUTO-RESPONSES */
	if (*flags & PURPLE_MESSAGE_AUTO_RESP)
	{
		/* The same idea as the code in src/server.c. */
		struct auto_response *ar = get_auto_response(connection, nick);
		time_t now = time(NULL);

		if ((now - ar->received) <= AUTO_RESPONSE_INTERVAL &&
		    !strcmp(msg, ar->message))
		{
			/* We've recently received an auto-response
			 * and it's the same as the one we've just received.
			 * Drop it!
			 */
			ar->received = now;
			return TRUE;
		}

		ar->received = now;
		g_free(ar->message);
		ar->message = g_strdup(msg);

		/* None of the other rules are for auto-responses. */
		return FALSE;
	}


	/* SIMPLE SUPPRESSION RULES */

	/* Suppress the FreeNode stats collection bot. */
	if (g_str_equal(nick, NICK_FREENODE_CONNECT))
	{
		return TRUE;
	}

	/* Suppress useless ChanServ notification. */
	if (g_str_equal(nick, NICK_CHANSERV) &&
	    g_str_has_prefix(msg, MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_CHANSERV_NO_OP_ACCESS))
	{

		return TRUE;
	}

	/* Suppress GameSurge message(s) of the day. */
	if (state & IRC_NETWORK_TYPE_GAMESURGE &&
	    g_str_equal(nick, NICK_GAMESURGE_GLOBAL))
	{
		return TRUE;
	}

	/* Suppress useless Jeux welcome. */
	if (g_str_equal(nick, NICK_JEUX_WELCOME))
	{
		return TRUE;
	}

	/* Suppress useless MemoServ notification. */
	if (g_str_equal(nick, NICK_MEMOSERV) &&
	    g_str_equal(msg, MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_MEMOSERV_NO_NEW_MEMOS))
	{

		return TRUE;
	}

	/* Suppress QuakeNet Q password warning. */
	if (g_str_equal(nick, NICK_QUAKENET_Q) &&
	    g_str_equal(msg, MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_QUAKENET_Q_CRUFT))
	{

		return TRUE;
	}

	/* Suppress Z registration notice. */
	if (g_str_equal(nick, "Z") &&
	    g_str_has_prefix(msg, MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_FRENCH_ADVERTISEMENT_START) &&
	    g_str_has_suffix(msg, MESSAGE_FRENCH_ADVERTISEMENT_END))
	{
		return TRUE;
	}

	/* Suppress Z's successful login notice. */
	if (g_str_equal(nick, "Z") &&
	    (g_str_has_prefix(msg, MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_FRENCH_LOGIN) ||
	    g_str_has_prefix(msg, MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_FRENCH_MOTD)))
	{
		return TRUE;
	}

	/* Suppress login message for the highest connection count. */
	if (g_str_has_prefix(msg, MESSAGE_PURPLE_NOTICE_PREFIX
	                          MESSAGE_LOGIN_CONNECTION_COUNT) ||
	    g_str_has_prefix(msg, MESSAGE_PURPLE_NOTICE_PREFIX
	                          MESSAGE_FRENCH_MAXIMUM_CONNECTION_COUNT))
	{
		return TRUE;
	}

	/* Suppress UnrealIRCd login cruft. */
	if (g_str_has_prefix(msg, MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_UNREAL_IRCD_HOSTNAME_FOUND) ||
	    g_str_equal(     msg, MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_UNREAL_IRCD_HOSTNAME_LOOKUP) ||
	    g_str_equal(     msg, MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_UNREAL_IRCD_IDENT_LOOKUP) ||
	    g_str_equal(     msg, MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_UNREAL_IRCD_IDENT_NO_RESPONSE) ||
	    g_str_has_prefix(msg, MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_UNREAL_IRCD_PONG_CRUFT))
	{

		return TRUE;
	}

	/* Suppress hostname cloak notification on Freenode. */
	if (g_str_has_suffix(nick, DOMAIN_SUFFIX_FREENODE) &&
	    g_str_has_prefix(msg, MESSAGE_PURPLE_NOTICE_PREFIX NICK_NICKSERV MESSAGE_NICKSERV_CLOAKED))
	{

		return TRUE;
	}


	/* SLIGHTLY COMPLICATED SUPPRESSION RULES */

	/* Supress QuakeNet and UnderNet Weird Login Cruft */
	{
		char temp;
		if (sscanf(msg, MESSAGE_PURPLE_NOTICE_PREFIX PATTERN_WEIRD_LOGIN_CRUFT, &temp) == 1)
		{

			return TRUE;
		}
	}

	/* Suppress silly notices of my own invites. */
	invite_prefix = g_strconcat(MESSAGE_PURPLE_NOTICE_PREFIX,
	                            purple_connection_get_display_name(connection), MESSAGE_INVITED, NULL);
	if (g_str_has_prefix(msg, invite_prefix))
	{
		g_free(invite_prefix);
		return TRUE;
	}
	g_free(invite_prefix);


	/* SERVICE MAGIC */

	/* Display ChanServ Access List Add Notifications in the chat window. */
	if (g_str_has_prefix(msg, MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_CHANSERV_ACCESS_LIST_ADD) &&
	    g_str_equal(nick, NICK_CHANSERV))
	{
		gchar *tmp;
		gchar *channel;
		gchar *level = NULL;
		gchar *msg2;

		channel = purple_markup_strip_html(msg + sizeof(MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_CHANSERV_ACCESS_LIST_ADD) - 1);
		if ((tmp = strchr(channel, ' ')) != NULL)
		{
			*tmp = '\0';
			if (g_str_has_prefix(tmp + 1, MESSAGE_CHANSERV_ACCESS_LIST_ADD_WITH_LEVEL))
			{
				/* The + 1 and - 1 are both kept to make it clear how that relates with other code. */
				level = tmp + 1 + sizeof(MESSAGE_CHANSERV_ACCESS_LIST_ADD_WITH_LEVEL) - 1;
				if ((tmp = strchr(level, ']')) != NULL)
					*tmp = '\0';
			}
		}

		if (NULL == level)
			msg2 = g_strdup(_("You have been added to the access list."));
		else
			msg2 = g_strdup_printf(_("You have been added to the access list with an access level of %s."), level);

		chat = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, channel, account);

		if (chat)
		{
			purple_conv_chat_write(PURPLE_CONV_CHAT(chat), nick,
			                     msg2, PURPLE_MESSAGE_SYSTEM, time(NULL));
			g_free(channel);
			g_free(msg2);
			return TRUE;
		}

		g_free(channel);
		g_free(msg2);
		return FALSE;
	}

	/* Display ChanServ Access List Delete Notifications in the chat window. */
	if (g_str_has_prefix(msg, MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_CHANSERV_ACCESS_LIST_DEL) &&
	    g_str_equal(nick, NICK_CHANSERV))
	{
		gchar *tmp;
		gchar *channel;

		channel = purple_markup_strip_html(msg + sizeof(MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_CHANSERV_ACCESS_LIST_DEL) - 1);
		if ((tmp = strchr(channel, ']')) != NULL)
			*tmp = '\0';

		chat = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, channel, account);

		if (chat)
		{
			purple_conv_chat_write(PURPLE_CONV_CHAT(chat), nick,
			                     _("You have been removed from the access list."),
			                     PURPLE_MESSAGE_SYSTEM, time(NULL));
			g_free(channel);
			return TRUE;
		}

		g_free(channel);
		return FALSE;
	}

	/* Display ChanServ channel join messages in the chat window. */
	if (g_str_has_prefix(msg, MESSAGE_PURPLE_NOTICE_PREFIX "[#") &&
	    (g_str_equal(nick, NICK_CHANSERV) || g_str_equal(nick, NICK_QUAKENET_L)))
	{
		gchar *msg2;
		gchar *msg3;
		gchar *channel;

		/* Duplicate the message so we can modify the string in place. */
		msg2 = g_strdup(msg);

		/* We need to keep a pointer to the beginning of the string so we can free it later. */
		msg3 = msg2;

		/* channel needs to start with the # */
		channel = msg3;
		channel += sizeof(MESSAGE_PURPLE_NOTICE_PREFIX);

		/* Find the "]", set it to the null character.
		 * This makes chan contain just the channel.
		 * Then, increment msg3 two spots so it contains
		 * just the message.
		 */
		if ((msg3 = g_strstr_len(msg3, strlen(msg3), "]")))
		{
			PurpleBlistNode *node;

			*msg3 = '\0';

			/* Make sure it's safe to increment the pointer */
			if ('\0' == msg3[1] || '\0' == msg3[2])
			{
				g_free(msg2);
				return FALSE;
			}

			msg3 += 2;

			if ((node = (PurpleBlistNode *)purple_blist_find_chat(account, channel)) != NULL)
			{
				const char *last_msg = purple_blist_node_get_string(node, PLUGIN_ID "_chanserv_join_msg");

				/* If we saw this the last time we joined, suppress it. */
				if (last_msg != NULL && strcmp(msg3, last_msg) == 0)
				{
					g_free(msg2);
					return TRUE;
				}
				else
					purple_blist_node_set_string(node, PLUGIN_ID "_chanserv_join_msg", msg3);
			}

			/* Write the message to the chat as a system message. */
			chat = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, channel, account);

			if (chat)
			{
				purple_conv_chat_write(PURPLE_CONV_CHAT(chat), nick,
				                     msg3, PURPLE_MESSAGE_SYSTEM, time(NULL));

				g_free(msg2);
				return TRUE;
			}
		}
		g_free(msg2);
		return FALSE;
	}

	/* Suppress useless NickServ notifications if we're identifying automatically. */
	if (state & IRC_NETWORK_TYPE_NICKSERV &&
	    (state & IRC_WILL_ID || state & IRC_KILLING_GHOST) &&
	    g_str_equal(nick, NICK_NICKSERV))
	{

		/* Track that the identification is finished. */
		if (g_str_equal(msg, MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_NICKSERV_IDENTIFIED))
			g_hash_table_insert(states, connection->proto_data,
			                    GINT_TO_POINTER((state & ~IRC_KILLING_GHOST & ~IRC_WILL_ID)
			                                    | IRC_DID_ID));

		/* The ghost has been killed, continue the signon. */
		if (state & IRC_KILLING_GHOST && strstr(msg, MESSAGE_GHOST_KILLED))
		{

			struct proto_stuff *stuff = g_new0(struct proto_stuff, 1);
			stuff->proto_data = connection->proto_data;
			stuff->account = account;

			ghosted_nickname_killed_cb(stuff);
		}

		/* The identification has failed. */
		if (g_str_equal(msg, MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_NICKSERV_ID_FAILURE))
		{
			g_hash_table_insert(states, connection->proto_data,
			                    GINT_TO_POINTER((state & ~IRC_KILLING_GHOST & ~IRC_WILL_ID)
			                                    | IRC_ID_FAILED));

			purple_notify_error(NULL,
			                  _("NickServ Authentication Error"),
			                  _("Error authenticating with NickServ"),
			                  _("Check your password."));
		}

		return TRUE;
	}

	/* Suppress useless AuthServ notifications if we're identifying automatically. */
	if (state & IRC_NETWORK_TYPE_GAMESURGE &&
	    state & IRC_WILL_ID &&
	    g_str_equal(nick, NICK_GAMESURGE_AUTHSERV))
	{

		/* Track that the identification is finished. */
		if (g_str_equal(msg, MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_GAMESURGE_AUTHSERV_IDENTIFIED))
			g_hash_table_insert(states, connection->proto_data,
			                    GINT_TO_POINTER((state & ~IRC_WILL_ID) | IRC_DID_ID));

		/* The identification has failed. */
		if (g_str_equal(msg, MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_GAMESURGE_AUTHSERV_ID_FAILURE))
		{
			g_hash_table_insert(states, connection->proto_data,
			                    GINT_TO_POINTER((state & ~IRC_WILL_ID) | IRC_ID_FAILED));

			purple_notify_error(NULL,
			                  _("GameSurge Authentication Error"),
			                  _("Error authenticating with AuthServ"),
			                  _("Check your password."));
		}

		return TRUE;
	}

	/* Suppress useless Q notifications if we're identifying automatically. */
	if (state & IRC_NETWORK_TYPE_QUAKENET &&
	    state & IRC_WILL_ID &&
	    g_str_equal(nick, NICK_QUAKENET_Q))
	{

		/* Track that the identification is finished. */
		if (g_str_equal(msg, MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_QUAKENET_Q_IDENTIFIED))
			g_hash_table_insert(states, connection->proto_data,
			                    GINT_TO_POINTER((state & ~IRC_WILL_ID) | IRC_DID_ID));

		/* The identification has failed. */
		if (g_str_equal(msg, MESSAGE_PURPLE_NOTICE_PREFIX MESSAGE_QUAKENET_Q_ID_FAILURE))
		{
			g_hash_table_insert(states, connection->proto_data,
			                    GINT_TO_POINTER((state & ~IRC_WILL_ID) | IRC_ID_FAILED));

			purple_notify_error(NULL,
			                  _("QuakeNet Authentication Error"),
			                  _("Error authenticating with Q"),
			                  _("Check your password."));
		}

		return TRUE;
	}

	return FALSE;
}


/*****************************************************************************
 * Plugin Code                                                               *
 *****************************************************************************/

static gboolean plugin_load(PurplePlugin *plugin)
{
	PurplePlugin *irc_prpl;
	PurplePluginProtocolInfo *prpl_info;
	PurpleAccountOption *option;
	void *conn_handle;
	void *conv_handle;

	irc_prpl = purple_plugins_find_with_id(IRC_PLUGIN_ID);

	if (NULL == irc_prpl)
		return FALSE;

	prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(irc_prpl);
	if (NULL == prpl_info)
		return FALSE;


	/* Create hash table. */
	states = g_hash_table_new(g_direct_hash, g_direct_equal);


	/* Register protocol preferences. */

	option = purple_account_option_string_new(_("Auth name"), PLUGIN_ID "_authname", "");
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);

	option = purple_account_option_string_new(_("Nick password"), PLUGIN_ID "_nickpassword", "");
	purple_account_option_set_masked(option, TRUE);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);

	option = purple_account_option_bool_new(_("Disconnect ghosts (Duplicate nicknames)"),
	                                      PLUGIN_ID "_disconnectghosts", 0);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);

	option = purple_account_option_string_new(_("Operator password"), PLUGIN_ID "_operpassword", "");
	purple_account_option_set_masked(option, TRUE);
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);


	/* Register callbacks. */

	conn_handle = purple_connections_get_handle();
	conv_handle = purple_conversations_get_handle();

	purple_signal_connect(conn_handle, "signed-on",
	                    plugin, PURPLE_CALLBACK(signed_on_cb), NULL);
	purple_signal_connect(conv_handle, "conversation-created",
	                    plugin, PURPLE_CALLBACK(conversation_created_cb), NULL);
	purple_signal_connect(conv_handle, "receiving-im-msg",
	                    plugin, PURPLE_CALLBACK(receiving_im_msg_cb), NULL);
	purple_signal_connect(conv_handle, "writing-chat-msg",
	                    plugin, PURPLE_CALLBACK(writing_chat_msg_cb), NULL);

	return TRUE;
}

static gboolean plugin_unload(PurplePlugin *plugin)
{
	PurplePlugin *irc_prpl;
	PurplePluginProtocolInfo *prpl_info;
	GList *list;

	irc_prpl = purple_plugins_find_with_id(IRC_PLUGIN_ID);
	if (NULL == irc_prpl)
		return FALSE;

	prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(irc_prpl);
	if (NULL == prpl_info)
		return FALSE;

	list = prpl_info->protocol_options;


	/* Remove protocol preferences. */
	while (NULL != list)
	{
		PurpleAccountOption *option = (PurpleAccountOption *) list->data;

		if (g_str_has_prefix(purple_account_option_get_setting(option), PLUGIN_ID "_"))
		{
			GList *llist = list;

			/* Remove this element from the list. */
			if (llist->prev)
				llist->prev->next = llist->next;
			if (llist->next)
				llist->next->prev = llist->prev;

			purple_account_option_destroy(option);

			list = g_list_next(list);

			g_list_free_1(llist);
		}
		else
			list = g_list_next(list);
	}

	return TRUE;
}

static void plugin_init(PurplePlugin *plugin)
{
	info.dependencies = g_list_append(info.dependencies, IRC_PLUGIN_ID);

#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("IRC Helper");
	info.summary = _("Handles the rough edges of the IRC protocol.");
	info.description = _("- Transparent authentication with a variety of "
			"services.\n- Suppression of various useless messages");
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, plugin_init, info)

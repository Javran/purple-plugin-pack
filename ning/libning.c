/*
 * libning
 *
 * libning is the property of its developers.  See the COPYRIGHT file
 * for more details.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "libning.h"
#include "ning_connection.h"
#include "ning_chat.h"

JsonObject *ning_json_parse(const gchar *data, gssize data_len)
{
	JsonParser *parser;
	JsonNode *root_node;
	JsonObject *root_obj;
	
	parser = json_parser_new();
	json_parser_load_from_data(parser, data, (gssize) data_len, NULL);
	
	root_node = json_parser_get_root(parser);
	root_obj =  json_node_dup_object(root_node);
	
	g_object_unref(parser);
	
	return root_obj;
}

gchar *
build_user_json(NingAccount *na)
{
	gchar *user_json;
	gchar *escaped_name;
	gchar *escaped_icon;
	gchar *escaped_id;
	
	if (na && na->name)
	{
		escaped_name = g_strescape(na->name, "");
	} else {
		escaped_name = g_strdup("");
	}
	if (na && na->icon_url)
	{
		escaped_icon = g_strescape(na->icon_url, "");
	} else {
		escaped_icon = g_strdup("");
	}
	if (na && na->ning_id)
	{
		escaped_id = g_strescape(na->ning_id, "");
	} else {
		escaped_id = g_strdup("");
	}
	
	user_json = g_strdup_printf("{\"name\":\"%s\",\"iconUrl\":\"%s\",\"isAdmin\":\"0\",\"ningId\":\"%s\",\"isNC\":\"0\"}", 
			escaped_name, escaped_icon, escaped_id);
	
	g_free(escaped_name);
	g_free(escaped_icon);
	g_free(escaped_id);
	
	return user_json;
}



/******************************************************************************/
/* PRPL functions */
/******************************************************************************/

static const char *ning_list_icon(PurpleAccount *account, PurpleBuddy *buddy)
{
	return "ning";
}


static GList *ning_statuses(PurpleAccount *account)
{
	GList *types = NULL;
	PurpleStatusType *status;
	
	purple_debug_info("ning", "statuses\n");
	
	/* Ning people are either online or offline */
	
	status = purple_status_type_new_full(PURPLE_STATUS_AVAILABLE, NULL, NULL, TRUE, TRUE, FALSE);
	types = g_list_append(types, status);
	
	status = purple_status_type_new_full(PURPLE_STATUS_OFFLINE, NULL, NULL, TRUE, TRUE, FALSE);
	types = g_list_append(types, status);
	
	purple_debug_info("ning", "statuses return\n");
	
	return types;
}

void ning_chat_login_cb(NingAccount *na, gchar *data, gsize data_len, gpointer userdata)
{
	JsonObject *obj;
	const gchar *result;
	const gchar *roomId;
	
	obj = ning_json_parse(data, data_len);

	purple_debug_info("ning", "chat_login_cb: %s\n", data?data:"(null)");
	
	if (data == NULL || data_len == 0)
		return;
	
	//{"command": "login","result": "ok","roomId": "thoughtleaders.thoughtleaders",
	// "count": 2,"token": "37lfxean70eqh_122d86d5cf7_6f95cb8e_122d49f8e48"}
	
	result = json_node_get_string(json_object_get_member(obj, "result"));
	if (!result || !g_str_equal(result, "ok"))
	{
		purple_connection_error(na->pc, _("Could not log on"));
		return;
	}
	purple_connection_update_progress(na->pc, _("Joining public chat"), 5, 5);
	purple_connection_set_state(na->pc, PURPLE_CONNECTED);
	
	g_free(na->chat_token);
	na->chat_token = g_strdup(json_node_get_string(json_object_get_member(obj, "token")));
	
	roomId = json_node_get_string(json_object_get_member(obj, "roomId"));
	
	ning_join_chat_by_name(na, roomId);
	
	json_object_unref(obj);
}

void ning_chat_redir_cb(NingAccount *na, gchar *data, gsize data_len, gpointer userdata)
{
	JsonObject *obj;
	gchar *postdata, *encoded_app, *encoded_id;
	gchar *user_json, *user_encoded;

	purple_debug_info("ning", "ning_chat_redir_cb: %s\n", data?data:"(null)");
	
	//We get a randomly generated chat domain to use
	// eg {"domain": "3841.chat07.ningim.com"}
	obj = ning_json_parse(data, data_len);
	
	g_free(na->chat_domain);
	na->chat_domain = g_strdup(json_node_get_string(json_object_get_member(obj, "domain")));
	
	json_object_unref(obj);
	
	//Use our new domain to log into the chat servers
	purple_connection_update_progress(na->pc, _("Logging into chat"), 4, 5);
	
	encoded_app = g_strdup(purple_url_encode(na->ning_app));
	encoded_id = g_strdup(purple_url_encode(na->ning_id));
	
	user_json = build_user_json(na);
	user_encoded = g_strdup(purple_url_encode(user_json));
	
	postdata = g_strdup_printf("a=%s&t=%s%s&i=%s&user=%s", encoded_app, encoded_app, encoded_id, encoded_id, user_encoded);
	ning_post_or_get(na, NING_METHOD_POST, na->chat_domain,
			"/xn/presence/login", postdata, ning_chat_login_cb, NULL, FALSE);
	
	g_free(postdata);
	g_free(encoded_app);
	g_free(encoded_id);
	g_free(user_encoded);
	g_free(user_json);
}

void ning_login_home_cb(NingAccount *na, gchar *data, gsize data_len, gpointer userdata)
{
	//We need to look for 
	//<script>window.bzplcm.add({"app":"thoughtleaders","user":"37lfxean70eqh"
	//and
	//xg.token = 'b1a7f3ce1719481334cdcc5fe8eabcaa';
	const gchar *start_string = "\nning = ";
	const gchar *mid_string = "}};\n";
	const gchar *xgtoken_start = "xg.token = '";
	gchar *tmp, *ning_json_string, *xg_token;
	gchar *url;
	JsonObject *obj, *profile;
	
	tmp = g_strstr_len(data, data_len, start_string);
	if (tmp == NULL)
	{
		purple_connection_error(na->pc, _("NingID not found"));
		return;
	}
	tmp += strlen(start_string);
	ning_json_string = g_strndup(tmp, strstr(tmp, mid_string) - tmp + 2);
	purple_debug_info("ning", "ning_json_string: %s\n", ning_json_string);
	
	obj = ning_json_parse(ning_json_string, strlen(ning_json_string));
	profile = json_node_get_object(json_object_get_member(obj, "CurrentProfile"));
	g_free(na->ning_id);
	na->ning_id = g_strdup(json_node_get_string(json_object_get_member(profile, "id")));
	g_free(na->name);
	na->name = g_strdup(json_node_get_string(json_object_get_member(profile, "fullName")));
	g_free(na->icon_url);
	na->icon_url = g_strdup_printf("%s&width=16&height=16", json_node_get_string(json_object_get_member(profile, "photoUrl")));
	
	tmp = g_strstr_len(data, data_len, xgtoken_start);
	if (tmp == NULL)
	{
		purple_connection_error(na->pc, _("xgToken not found"));
		return;
	}
	tmp += strlen(xgtoken_start);
	xg_token = g_strndup(tmp, strchr(tmp, '\'') - tmp);
	g_free(na->xg_token);
	na->xg_token = xg_token;
	
	//Now we should have everything we need to sign into chat
	purple_connection_update_progress(na->pc, _("Fetching chat server"), 3, 5);
	
	url = g_strdup_printf("/xn/redirector/redirect?a=%s", purple_url_encode(na->ning_app));
	ning_post_or_get(na, NING_METHOD_GET, "chat01.ningim.com",
					 url, NULL, ning_chat_redir_cb, NULL, FALSE);
	g_free(url);
}

void ning_scan_cookies_for_id(gchar *key, gchar *value, NingAccount *na)
{
	if (g_str_has_prefix(key, "xn_id_"))
	{
		g_free(na->ning_app);
		na->ning_app = g_strdup(&key[6]);
	}
}

static void ning_login_cb(NingAccount *na, gchar *response, gsize len,
						 gpointer userdata)
{
	purple_connection_update_progress(na->pc, _("Fetching token"), 2, 4);
	
	// ok, we're logged into the host website now
	
	// Pull the host's Ning account id from the cookie
	g_hash_table_foreach(na->cookie_table, (GHFunc)ning_scan_cookies_for_id, na);
	
	//Load the homepage to grab the interesting info
	ning_post_or_get(na, NING_METHOD_GET, purple_account_get_string(na->account, "host", NULL),
					"/", NULL, ning_login_home_cb, NULL, FALSE);
}

static void ning_login(PurpleAccount *account)
{
	NingAccount *na;
	gchar *postdata, *encoded_username, *encoded_password;
	gchar *encoded_host, *url;
	const gchar *host;
	
	purple_debug_info("ning", "login\n");
	
	/* Create account and initialize state */
	na = g_new0(NingAccount, 1);
	na->account = account;
	na->pc = purple_account_get_connection(account);
	
	na->last_messages_download_time = time(NULL) - 60; /* 60 secs is a safe buffer */
	
	na->cookie_table = g_hash_table_new_full(g_str_hash, g_str_equal,
											  g_free, g_free);
	na->hostname_ip_cache = g_hash_table_new_full(g_str_hash, g_str_equal,
												   g_free, g_free);
	
	g_hash_table_replace(na->cookie_table, g_strdup("xg_cookie_check"),
						 g_strdup("1"));
	
	account->gc->proto_data = na;
	
	purple_connection_set_state(na->pc, PURPLE_CONNECTING);
	purple_connection_update_progress(na->pc, _("Logging in"), 1, 4);
	
	encoded_username = g_strdup(purple_url_encode(purple_account_get_username(account)));
	encoded_password = g_strdup(purple_url_encode(purple_account_get_password(account)));
	
	postdata = g_strdup_printf("xg_token=&emailAddress=%s&password=%s",
							   encoded_username, encoded_password);
	g_free(encoded_username);
	g_free(encoded_password);
	
	host = purple_account_get_string(account, "host", NULL);
	if (host == NULL || host[0] == '\0')
	{
		purple_connection_error(na->pc, _("Host not set"));
		return;
	}
	
	encoded_host = g_strdup(purple_url_encode(host));
	url = g_strdup_printf("/main/authorization/doSignIn?target=http%%3A%%2F%%2F%s", host); 
	
	ning_post_or_get(na, NING_METHOD_POST | NING_METHOD_SSL, host,
					url, postdata, ning_login_cb, NULL, FALSE);
	g_free(postdata);
}

static void ning_close(PurpleConnection *pc)
{
	NingAccount *na;
	gchar *postdata;
	gchar *host_encoded;
	gchar *xg_token_encoded;
	PurpleDnsQueryData *dns_query;
	
	purple_debug_info("ning", "disconnecting account\n");
	
	na = pc->proto_data;
	
	host_encoded = g_strdup(purple_url_encode(purple_account_get_string(na->account, "host", "")));
	xg_token_encoded = g_strdup(purple_url_encode(na->xg_token));
	
	postdata = g_strdup_printf("target=%s&xg_token=%s", host_encoded, xg_token_encoded);
	
	ning_post_or_get(na, NING_METHOD_POST, purple_account_get_string(na->account, "host", NULL),
					 "/main/authorization/signOut", postdata, NULL, NULL, FALSE);
	
	g_free(host_encoded);
	g_free(xg_token_encoded);
	g_free(postdata);
	
	purple_debug_info("ning", "destroying %d incomplete connections\n",
					  g_slist_length(na->conns));
	
	while (na->conns != NULL)
		ning_connection_destroy(na->conns->data);
	
	while (na->dns_queries != NULL) {
		dns_query = na->dns_queries->data;
		purple_debug_info("ning", "canceling dns query for %s\n",
						  purple_dnsquery_get_host(dns_query));
		na->dns_queries = g_slist_remove(na->dns_queries, dns_query);
		purple_dnsquery_destroy(dns_query);
	}
	
	g_hash_table_destroy(na->cookie_table);
	g_hash_table_destroy(na->hostname_ip_cache);
	
	while (na->chats != NULL)
	{	
		NingChat *chat = na->chats->data;
		na->chats = g_list_remove(na->chats, chat);
		purple_timeout_remove(chat->userlist_timer);
		purple_timeout_remove(chat->message_poll_timer);
		purple_conv_chat_left(PURPLE_CONV_CHAT(purple_find_chat(pc, chat->purple_id)));
		g_free(chat->roomId);
		g_free(chat->ning_hash);
		g_free(chat);
	}
	
	g_free(na->ning_id);
	g_free(na->name);
	g_free(na->icon_url);
	g_free(na->xg_token);
	g_free(na->ning_app);
	
	g_free(na);
}

#if PURPLE_MAJOR_VERSION >= 2 && PURPLE_MINOR_VERSION >= 5
static GHashTable *ning_get_account_text_table(PurpleAccount *account)
{
	GHashTable *table;

	table = g_hash_table_new(g_str_hash, g_str_equal);

	g_hash_table_insert(table, "login_label", (gpointer)_("Email Address..."));

	return table;
}
#endif

void
ning_change_passwd(PurpleConnection *pc, const char *old_pass, const char *new_pass)
{
	NingAccount *na;
	PurpleAccount *account;
	gchar *encoded_username;
	gchar *encoded_password;
	gchar *encoded_token;
	gchar *postdata;
	
	if (pc == NULL)
		return;
	na = pc->proto_data;
	if (na == NULL || na->xg_token == NULL)
		return;
	account = pc->account;
	if (account == NULL)
		return;
	
	encoded_username = g_strdup(purple_url_encode(purple_account_get_username(account)));
	encoded_password = g_strdup(purple_url_encode(new_pass));
	encoded_token = g_strdup(purple_url_encode(na->xg_token));
	
	postdata = g_strdup_printf("emailAddress=%s&password=%s&xg_token=%s",
							   encoded_username, encoded_password, encoded_token);
	
	ning_post_or_get(na, NING_METHOD_POST, purple_account_get_string(na->account, "host", NULL),
					 "/profiles/settings/updateEmailAddress", postdata, NULL, NULL, FALSE);
	
	g_free(postdata);
	g_free(encoded_token);
	g_free(encoded_password);
	g_free(encoded_username);
}


/******************************************************************************/
/* Plugin functions */
/******************************************************************************/

static gboolean plugin_load(PurplePlugin *plugin)
{
	return TRUE;
}

static gboolean plugin_unload(PurplePlugin *plugin)
{
	return TRUE;
}

static void plugin_init(PurplePlugin *plugin)
{
	PurpleAccountOption *option;
	PurplePluginInfo *info = plugin->info;
	PurplePluginProtocolInfo *prpl_info = info->extra_info;
	
	option = purple_account_option_string_new("Host", "host", "");
	prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
	
}

static PurplePluginProtocolInfo prpl_info = {
	/* options */
	0,

	NULL,                   /* user_splits */
	NULL,                   /* protocol_options */
	NO_BUDDY_ICONS          /* icon_spec */
	/*{"jpg", 0, 0, 50, 50, -1, PURPLE_ICON_SCALE_SEND}*/, /* icon_spec */
	ning_list_icon,         /* list_icon */
	NULL,                   /* list_emblems */
	NULL,                   /* status_text */
	NULL,                   /* tooltip_text */
	ning_statuses,          /* status_types */
	NULL,                   /* blist_node_menu */
	NULL,                   /* chat_info */
	NULL,                   /* chat_info_defaults */
	ning_login,             /* login */
	ning_close,             /* close */
	ning_send_im,           /* send_im */
	NULL,                   /* set_info */
	NULL,                   /* send_typing */
	NULL,                   /* get_info */
	NULL,                   /* set_status */
	NULL,                   /* set_idle */
	ning_change_passwd,     /* change_passwd */
	NULL,                   /* add_buddy */
	NULL,                   /* add_buddies */
	NULL,                   /* remove_buddy */
	NULL,                   /* remove_buddies */
	NULL,                   /* add_permit */
	NULL,                   /* add_deny */
	NULL,                   /* rem_permit */
	NULL,                   /* rem_deny */
	NULL,                   /* set_permit_deny */
	ning_join_chat,         /* join_chat */
	NULL,                   /* reject chat invite */
	NULL,                   /* get_chat_name */
	NULL,                   /* chat_invite */
	NULL,                   /* chat_leave */
	ning_chat_whisper,      /* chat_whisper */
	ning_chat_send,         /* chat_send */
	NULL,                   /* keepalive */
	NULL,                   /* register_user */
	NULL,                   /* get_cb_info */
	NULL,                   /* get_cb_away */
	NULL,                   /* alias_buddy */
	NULL,                   /* group_buddy */
	NULL,                   /* rename_group */
	NULL,                   /* buddy_free */
	NULL,                   /* convo_closed */
	purple_normalize_nocase,/* normalize */
	NULL,                   /* set_buddy_icon */
	NULL,                   /* remove_group */
	NULL,                   /* get_cb_real_name */
	NULL,                   /* set_chat_topic */
	NULL,                   /* find_blist_chat */
	NULL,                   /* roomlist_get_list */
	NULL,                   /* roomlist_cancel */
	NULL,                   /* roomlist_expand_category */
	NULL,                   /* can_receive_file */
	NULL,                   /* send_file */
	NULL,                   /* new_xfer */
	NULL,                   /* offline_message */
	NULL,                   /* whiteboard_prpl_ops */
	NULL,                   /* send_raw */
	NULL,                   /* roomlist_room_serialize */
	NULL,                   /* unregister_user */
	NULL,                   /* send_attention */
	NULL,                   /* attention_types */
#if PURPLE_MAJOR_VERSION >= 2 && PURPLE_MINOR_VERSION >= 5
	sizeof(PurplePluginProtocolInfo), /* struct_size */
	ning_get_account_text_table, /* get_account_text_table */
#else
   (gpointer) sizeof(PurplePluginProtocolInfo)
#endif
};

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	2, /* major_version */
	3, /* minor version */
	PURPLE_PLUGIN_PROTOCOL, /* type */
	NULL, /* ui_requirement */
	0, /* flags */
	NULL, /* dependencies */
	PURPLE_PRIORITY_DEFAULT, /* priority */
	"prpl-bigbrownchunx-ning", /* id */
	"Ning", /* name */
	NING_PLUGIN_VERSION, /* version */
	N_("Ning Protocol Plugin"), /* summary */
	N_("Ning Protocol Plugin"), /* description */
	"Eion Robb <eionrobb@gmail.com>", /* author */
	"", /* homepage */
	plugin_load, /* load */
	plugin_unload, /* unload */
	NULL, /* destroy */
	NULL, /* ui_info */
	&prpl_info, /* extra_info */
	NULL, /* prefs_info */
	NULL, /* actions */

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

PURPLE_INIT_PLUGIN(ning, plugin_init, info);

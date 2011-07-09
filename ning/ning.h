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

#ifndef LIBNING_H
#define LIBNING_H

#include "../common/pp_internal.h"

#define NING_TEMP_GROUP_NAME "Ning Temp"

#include <glib.h>

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <json-glib/json-glib.h>

#ifndef G_GNUC_NULL_TERMINATED
#	if __GNUC__ >= 4
#		define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
#	else
#		define G_GNUC_NULL_TERMINATED
#	endif /* __GNUC__ >= 4 */
#endif /* G_GNUC_NULL_TERMINATED */

#ifdef _WIN32
#	include "win32dep.h"
#	define dlopen(a,b) LoadLibrary(a)
#	define RTLD_LAZY
#	define dlsym(a,b) GetProcAddress(a,b)
#	define dlclose(a) FreeLibrary(a)
#else
#	include <arpa/inet.h>
#	include <dlfcn.h>
#	include <netinet/in.h>
#	include <sys/socket.h>
#endif

#ifndef PURPLE_PLUGINS
#	define PURPLE_PLUGINS
#endif

#include "accountopt.h"
#include "connection.h"
#include "debug.h"
#include "dnsquery.h"
#include "proxy.h"
#include "prpl.h"
#include "request.h"
#include "sslconn.h"
#include "version.h"

typedef struct _NingAccount NingAccount;

struct _NingAccount {
	PurpleAccount *account;
	PurpleConnection *pc;
	GHashTable *hostname_ip_cache;
	GSList *conns; /**< A list of all active connections */
	GSList *dns_queries;
	GList *chats;
	GHashTable *cookie_table;
	
	time_t last_messages_download_time;
	
	gchar *xg_token;
	
	gchar *ning_app;
	gchar *ning_id;
	gchar *name;
	gchar *icon_url;
	
	gchar *chat_domain;
	gchar *chat_token;
};

JsonObject *ning_json_parse(const gchar *data, gssize data_len);
gchar *build_user_json(NingAccount *na);


#endif /* LIBNING_H */

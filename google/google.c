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

#include <errno.h>
#include <string.h>

#include <cmds.h>
#include <conversation.h>
#include <debug.h>
#include <plugin.h>
#include <proxy.h>
#include <util.h>

static PurpleCmdId google_cmd_id = 0;

#define GOOGLE_URL_FORMAT	"http://%s/search?q=%s&btnI=I%%27m+Feeling+Lucky"

/******************************************************************************
 * Structs
 *****************************************************************************/
typedef struct {
	PurpleConversation *conv;
	gchar *host;
	gint port;
	gchar *path;

	gchar *request;
	gsize request_written;

	gint fd;
	gint inpa;

	GString *response;

	PurpleProxyConnectData *conn_data;
} GoogleFetchUrlData;

/******************************************************************************
 * GoogleFetchUrlData API
 *****************************************************************************/
static GoogleFetchUrlData *
google_fetch_url_data_new(const gchar *url) {
	GoogleFetchUrlData *gfud = g_new0(GoogleFetchUrlData, 1);

	if(!purple_url_parse(url, &gfud->host, &gfud->port, &gfud->path, NULL,
						 NULL))
	{
		g_free(gfud);
		return NULL;
	}

	gfud->response = g_string_new("");
	
	return gfud;
}

static void
google_fetch_url_data_free(GoogleFetchUrlData *gfud) {
	g_free(gfud->host);
	g_free(gfud->path);

	g_free(gfud->request);

	g_string_free(gfud->response, TRUE);
	
	if(gfud->inpa > 0)
		purple_input_remove(gfud->inpa);

	if(gfud->fd >= 0)
		close(gfud->fd);

	if(gfud->conn_data)
		purple_proxy_connect_cancel(gfud->conn_data);

	g_free(gfud);
}

/******************************************************************************
 * The final result (hiding in the middle, very sneaky)
 *****************************************************************************/
static void
google_output_url(GoogleFetchUrlData *gfud) {
	gchar *str = NULL, *url_s = NULL, *url_e = NULL;
	gsize len = 0;
	const gchar *needle = "Location: ";

	/* if our conv has disappeared from under us, drop out */
	if(!gfud->conv)
		return;

	str = gfud->response->str;
	len = gfud->response->len;

	url_s = g_strstr_len(str, len, needle);
	if(!url_s)
		return;

	len = strlen(url_s);
	url_s += strlen(needle);

	url_e = g_strstr_len(url_s, len, "\r\n");
	if(!url_e)
		return;

	*url_e = '\0';

	if(gfud->conv->type == PURPLE_CONV_TYPE_IM)
		purple_conv_im_send(PURPLE_CONV_IM(gfud->conv), url_s);
	else if(gfud->conv->type == PURPLE_CONV_TYPE_CHAT)
		purple_conv_chat_send(PURPLE_CONV_CHAT(gfud->conv), url_s);
}

/******************************************************************************
 * URL Stuff
 *****************************************************************************/
static void
im_feeling_lucky_recv_cb(gpointer data, gint source, PurpleInputCondition c) {
	GoogleFetchUrlData *gfud = (GoogleFetchUrlData *)data;
	gint len;
	gchar buff[4096];

	while((len = read(source, buff, sizeof(buff))) > 0)
		gfud->response = g_string_append_len(gfud->response, buff, len);

	if(len < 0) {
		if(errno == EAGAIN)
			return;

		/* need to die here */

		return;
	}

	if(len == 0) {
		google_output_url(gfud);
		google_fetch_url_data_free(gfud);
	}
}

static void
im_feeling_lucky_send_cb(gpointer data, gint source, PurpleInputCondition c) {
	GoogleFetchUrlData *gfud = (GoogleFetchUrlData *)data;
	gint len, total_len;

	total_len = strlen(gfud->request);

	len = write(gfud->fd, gfud->request + gfud->request_written,
				total_len - gfud->request_written);

	if(len < 0) {
		if(errno == EAGAIN)
			return;

		/* need to die here */

		return;
	}

	gfud->request_written += len;

	if(gfud->request_written < total_len)
		return;

	/* done writing the request, now read the response */
	purple_input_remove(gfud->inpa);
	gfud->inpa = purple_input_add(gfud->fd, PURPLE_INPUT_READ,
								  im_feeling_lucky_recv_cb, gfud);
}

static void
im_feeling_lucky_cb(gpointer data, gint source, const gchar *e) {
	GoogleFetchUrlData *gfud = (GoogleFetchUrlData *)data;

	gfud->conn_data = NULL;

	if(source == -1) {
		purple_debug_error("google", "unable to connect to %s: %s\n",
						   gfud->host, gfud->path);

		google_fetch_url_data_free(gfud);

		return;
	}

	gfud->fd = source;

	gfud->request = g_strdup_printf(
		"GET /%s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"User-Agent: Purple/%u.%u.%u\r\n"
		"Accept: */*\r\n"
		"Connection: close\r\n"
		"Referer: %s\r\n"
		"\r\n",
		gfud->path,
		gfud->host,
		purple_major_version, purple_minor_version, purple_micro_version,
		gfud->host);

	gfud->inpa = purple_input_add(gfud->fd, PURPLE_INPUT_WRITE,
								  im_feeling_lucky_send_cb, gfud);
	im_feeling_lucky_send_cb(gfud, gfud->fd, PURPLE_INPUT_WRITE);
}

/******************************************************************************
 * Command stuff
 *****************************************************************************/
static PurpleCmdRet
im_feeling_lucky(PurpleConversation *conv, const gchar *cmd, gchar **args,
				 gchar *error, void *data)
{
	GoogleFetchUrlData *gfud = NULL;
	PurplePlugin *plugin = (PurplePlugin *)data;
	gchar *url = NULL;

	url = g_strdup_printf(GOOGLE_URL_FORMAT, "www.google.com",
						  purple_url_encode(args[0]));
	gfud = google_fetch_url_data_new(url);
	g_free(url);

	if(!gfud)
		return PURPLE_CMD_RET_FAILED;

	gfud->conv = conv;

	/* now make the connection */
	gfud->conn_data =
		purple_proxy_connect(plugin, NULL, gfud->host, gfud->port,
							 im_feeling_lucky_cb, gfud);

	if(!gfud->conn_data) {
		google_fetch_url_data_free(gfud);
		
		return PURPLE_CMD_RET_FAILED;
	}

	return PURPLE_CMD_RET_OK;
}

/******************************************************************************
 * Plugin Stuff
 *****************************************************************************/
static gboolean
plugin_load(PurplePlugin *plugin) {
	google_cmd_id =
		purple_cmd_register("google", "s", PURPLE_CMD_P_PLUGIN,
							PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT,
							NULL, PURPLE_CMD_FUNC(im_feeling_lucky),
							_("Returns the url for a Google I'm feeling lucky "
							  "search"),
							plugin);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin) {
	purple_cmd_unregister(google_cmd_id);

	return TRUE;
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,

	"core-plugin_pack-google",
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
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

static void
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Google");
	info.summary = _("Returns the url for a Google \"I'm feeling lucky\" search");
	info.description = info.summary;
}

PURPLE_INIT_PLUGIN(google, init_plugin, info)

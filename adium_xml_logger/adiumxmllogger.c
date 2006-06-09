/*
 * Adium XML Logger - Implements version 0.2 of Adium's XML logging format
 * Copyright (C) 2005 Gary Kramlich <grim@reaperworld.com>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#if HAVE_CONFIG_H
# include "../gpp_config.h"
#endif /* HAVE_CONFIG_H */

#define GAIM_PLUGINS

#define PLUGIN_ID 			"core-plugin_pack-adium_xml_logger"
#define ADIUM_XML_VERSION 	"0.2"
#define ADIUM_XML_INTERVAL	(30000)

#include <string.h>
#include <sys/stat.h>

#include <account.h>
#include <debug.h>
#include <log.h>
#include <prefs.h>
#include <util.h>
#include <version.h>
#include <xmlnode.h>

#include "../common/i18n.h"

/******************************************************************************
 * Globals
 *****************************************************************************/
static GaimLogLogger *adium_xml_logger;

/******************************************************************************
 * Structs
 *****************************************************************************/
#define ADIUM_XML_LOGGER_DATA(obj)		((AdiumXMLLoggerData *)(obj))

typedef struct {
	xmlnode *root;
	guint scheduler;
} AdiumXMLLoggerData;

/******************************************************************************
 * Saving stuff
 *****************************************************************************/
static void
adium_xml_logger_save(AdiumXMLLoggerData *data) {
	gchar *output;

	if(data->scheduler != 0) {
		g_source_remove(data->scheduler);
		data->scheduler = 0;
	}
		
	output = xmlnode_to_formatted_str(data->root, NULL);
	gaim_debug_info("AdiumXMLLogger", "%s\n\n", output);
	g_free(output);
}

static gboolean
adium_xml_logger_schedule_save_cb(gpointer d) {
	AdiumXMLLoggerData *data = ADIUM_XML_LOGGER_DATA(d);

	/* set the scheduler to 0 before the call to save which will attempt to
	 * remove the scheduler if it's not 0.  But since the timeout is going to
	 * get removed when we return false, that would be unncessary.
	 */
	data->scheduler = 0;

	adium_xml_logger_save(data);

	return FALSE;
}

static void
adium_xml_logger_schedule_save(AdiumXMLLoggerData *data) {
	if(data->scheduler != 0)
		return;

	gaim_debug_info("AdiumXMLLogger", "scheduling save...\n");

	data->scheduler = g_timeout_add(ADIUM_XML_INTERVAL,
									adium_xml_logger_schedule_save_cb,
									data);
}

/******************************************************************************
 * Logger stuff
 *****************************************************************************/
static void
adium_xml_logger_create(GaimLog *log) {
	AdiumXMLLoggerData *data;
	GaimAccount *account;
	const gchar *username, *protocol;
	gchar buff[80];
	time_t t;

	if(!log && log->type != GAIM_LOG_IM)
		return;

	/* grab the account to be used later */
	account = gaim_conversation_get_account(log->conv);

	log->logger_data = data = g_new0(AdiumXMLLoggerData, 1);
	
	/* build the root node */
	data->root = xmlnode_new("chat");

	/* add the start time */
	t = time(NULL);
	strftime(buff, sizeof(buff), "%Y-%m-%d", localtime(&t));
	xmlnode_set_attrib(data->root, "startdate", buff);
	
	/* add the account */
	username = gaim_account_get_username(account);
	xmlnode_set_attrib(data->root, "account", username);
	
	/* add the service */
	protocol = gaim_account_get_protocol_name(account);
	xmlnode_set_attrib(data->root, "service", protocol);

	/* add the adium xml version */
	xmlnode_set_attrib(data->root, "version", ADIUM_XML_VERSION);

	/* schedule a save */
	adium_xml_logger_schedule_save(data);
}

static void
adium_xml_logger_write(GaimLog *log, GaimMessageFlags type, const char *from,
					   time_t time, const char *message)
{
	AdiumXMLLoggerData *data = ADIUM_XML_LOGGER_DATA(log->logger_data);
	xmlnode *msg;
	gchar buff[80];

	if(!log && log->type != GAIM_LOG_IM)
		return;

	/* create the message element */
	msg = xmlnode_new_child(data->root, "message");

	/* add the sender attribute */
	xmlnode_set_attrib(msg, "sender", from);

	/* add the timestamp attribute */
	strftime(buff, sizeof(buff), "%s", localtime(&time));
	xmlnode_set_attrib(msg, "time", buff);

	/* add the message data */
	xmlnode_insert_data(msg, message, -1);

	/* schedule a write */
	adium_xml_logger_schedule_save(data);
}

static void
adium_xml_logger_finalize(GaimLog *log) {
	AdiumXMLLoggerData *data = ADIUM_XML_LOGGER_DATA(log->logger_data);
	gchar buff[80];
	time_t t;

	t = time(NULL);
	strftime(buff, sizeof(buff), "%Y-%m-%d", localtime(&t));
	xmlnode_set_attrib(data->root, "enddata", buff);

	adium_xml_logger_save(data);

	xmlnode_free(data->root);

	g_free(data);
	log->logger_data = NULL;
}

/******************************************************************************
 * Plugin stuff
 *****************************************************************************/
static gboolean
plugin_load(GaimPlugin *plugin)
{
	adium_xml_logger =
		gaim_log_logger_new("adium-xml", N_("Adium XML"), 3,
							adium_xml_logger_create,
							adium_xml_logger_write,
							adium_xml_logger_finalize);
	gaim_log_logger_add(adium_xml_logger);

	gaim_prefs_trigger_callback("/core/logging/format");

	return TRUE;
}

static gboolean
plugin_unload(GaimPlugin *plugin)
{
	gaim_log_logger_remove(adium_xml_logger);

	gaim_prefs_trigger_callback("/core/logging/format");

	return TRUE;
}

static GaimPluginInfo info =
{
	GAIM_PLUGIN_MAGIC,								/**< magic			*/
	GAIM_MAJOR_VERSION,								/**< major version	*/
	GAIM_MINOR_VERSION,								/**< minor version	*/
	GAIM_PLUGIN_STANDARD,							/**< type			*/
	NULL,											/**< ui_requirement	*/
	0,												/**< flags			*/
	NULL,											/**< dependencies	*/
	GAIM_PRIORITY_DEFAULT,							/**< priority		*/

	PLUGIN_ID,										/**< id				*/
	NULL,											/**< name			*/
	GPP_VERSION,									/**< version		*/
	NULL,											/**< summary		*/
	NULL,											/**< description	*/
	"Gary Kramich <grim@reaperworld.com>",			/**< author			*/
	GPP_WEBSITE,									/**< homepage		*/

	plugin_load,									/**< load			*/
	plugin_unload,									/**< unload			*/
	NULL,											/**< destroy		*/

	NULL,											/**< ui_info		*/
	NULL,											/**< extra_info		*/
	NULL,											/**< prefs_info		*/
	NULL											/**< actions		*/
};

static void
init_plugin(GaimPlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GPP_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GPP_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Adium XML Logger");
	info.summary = _("Implements version 0.2 of Adium's XML Logging");
	info.description = _("Implements version 0.2 of Adium's XML Logging");
}

GAIM_INIT_PLUGIN(adium_xml_logger, init_plugin, info)

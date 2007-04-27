/*
 * Old Logger - Re-implements the legacy, deficient, logging
 * Copyright (C) 2004 Stu Tomlinson <stu@nosnilmot.com>
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
#ifdef HAVE_CONFIG_H
# include "../pp_config.h"
#endif /* HAVE_CONFIG_H */

#define PURPLE_PLUGINS
#define OLDLOGGER_PLUGIN_ID "core-plugin_pack-oldlogger"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <account.h>
#include <debug.h>
#include <log.h>
#include <prefs.h>
#include <prpl.h>
#include <stringref.h>
#include <util.h>
#include <version.h>

#include "../common/i18n.h"

#include <glib.h>

/* We want to use the gstdio functions when possible so that non-ASCII
 * filenames are handled properly on Windows. */
#if GLIB_CHECK_VERSION(2,6,0)
#include <glib/gstdio.h>
#else
#include <sys/stat.h>
#define g_fopen fopen
#define g_rename rename
#define g_stat stat
#define g_unlink unlink
#endif


#if PURPLE_VERSION_CHECK(2,0,0)
static PurpleLogLogger *oldtxt_logger;
static PurpleLogLogger *oldhtml_logger;
#define return_written return written
#else
static PurpleLogLogger oldtxt_logger;
static PurpleLogLogger oldhtml_logger;
#define return_written return
#endif

struct basic_logger_data {
	FILE *file;
	char *filename;
	gboolean new;
	long offset;
	time_t old_last_modified;
};

static const gchar *
oldlogger_date_full(void)
{
	gchar *buf;
	time_t tme;

	time(&tme);
	buf = ctime(&tme);
	buf[strlen(buf) - 1] = '\0';

	return buf;
}

static void old_logger_create(PurpleLog *log)
{
	if(log->type == PURPLE_LOG_SYSTEM){
		const char *ud = purple_user_dir();
		char *dir;
		char *filename;
		struct basic_logger_data *data;
		struct stat st;

		dir = g_build_filename(ud, "logs", NULL);
		purple_build_dir (dir, S_IRUSR | S_IWUSR | S_IXUSR);
		filename = g_build_filename(dir, "system", NULL);
		g_free(dir);

		log->logger_data = data = g_new0(struct basic_logger_data, 1);

		if (g_stat(filename, &st) < 0)
			data->new = TRUE;
		else
			data->old_last_modified = st.st_mtime;

		data->file = g_fopen(filename, "a");
		if (!data->file) {
			purple_debug(PURPLE_DEBUG_ERROR, "log",
					"Could not create log file %s\n", filename);
			g_free(filename);
			g_free(data);
			return;
		}
		data->filename = filename;
		data->offset = ftell(data->file);
	}
}

static void old_logger_update_index(PurpleLog *log)
{
	struct basic_logger_data *data = log->logger_data;
	struct stat st;
	char *index_path;
	char *index_data;
	GError *error;
	int index_fd;
	char *index_tmp;
	FILE *index;

	g_return_if_fail(data->offset > 0);

	index_path = g_strdup(data->filename);
	/* Change the .log extension to .idx */
	strcpy(index_path + strlen(index_path) - 3, "idx");
	if (!data->new && (g_stat(index_path, &st) || st.st_mtime < data->old_last_modified))
	{
		g_free(index_path);
		return;
	}

	/* The index file exists and is at least as new as the log, so open it. */
	if (!data->new && !g_file_get_contents(index_path, &index_data, NULL, &error))
	{
		purple_debug_error("log", "Failed to read contents of index \"%s\": %s\n",
						 index_path, error->message);
		g_error_free(error);
		g_free(index_path);
		return;
	}
	if (data->new)
		index_data = g_strdup("");

	index_tmp = g_strdup_printf("%s.XXXXXX", index_path);
	if ((index_fd = g_mkstemp(index_tmp)) == -1) {
		purple_debug_error("log", "Failed to open index temp file: %s\n",
		                 strerror(errno));
		g_error_free(error);
		g_free(index_path);
		g_free(index_data);
		g_free(index_tmp);
		return;
	} else {
		if ((index = fdopen(index_fd, "wb")) == NULL)
		{
			purple_debug_error("log", "Failed to fdopen() index temp file: %s\n",
			                 strerror(errno));
			close(index_fd);
			if (index_tmp != NULL)
			{
				g_unlink(index_tmp);
				g_free(index_tmp);
			}
			g_free(index_path);
			g_free(index_data);
			return;
		}
	}

	fprintf(index, "%s", index_data);
	fprintf(index, "%ld\t%ld\t%lu\n", data->offset, ftell(data->file) - data->offset, (unsigned long)log->time);
	fclose(index);

	if (g_rename(index_tmp, index_path))
	{
		purple_debug_warning("log", "Failed to rename index temp file \"%s\" to \"%s\": %s\n",
						   index_tmp, index_path, strerror(errno));
		g_unlink(index_tmp);
		g_free(index_tmp);
	}
}

static void old_logger_finalize(PurpleLog *log)
{
	struct basic_logger_data *data = log->logger_data;
	if (data) {
		if(data->file)
			fflush(data->file);

		old_logger_update_index(log);

		if(data->file)
			fclose(data->file);

		g_free(data->filename);
		g_free(data);
	}
}

/*******************************
 ** Ye Olde PLAIN TEXT LOGGER **
 *******************************/

#if PURPLE_VERSION_CHECK(2,0,0)
static gsize
#else
static void
#endif
oldtxt_logger_write(PurpleLog *log, PurpleMessageFlags type,
			     const char *from, time_t time, const char *message)
{
	char date[64];
	char *stripped = NULL;
	struct basic_logger_data *data = log->logger_data;
	const char *prpl = (purple_find_prpl(purple_account_get_protocol_id(log->account)))->info->name;
	gsize written = 0;
	if (!data) {
		/* This log is new.  We could use the logger's 'new' function, but
		 * creating a new file there would result in empty files in the case
		 * that you open a convo with someone, but don't say anything.
		 *
		 * The log is also not system log. Because if it is, data would be
		 * created in old_logger_create
		 * Stu: well, this isn't really necessary with crappy old logging, but I'm
		 * too lazy to do it any other way.
		 */
		const char *ud = purple_user_dir();
		char *filename;
		char *guy = g_strdup(purple_normalize(log->account, log->name));
		char *chat;
		char *dir;
		char *logfile;
		struct stat st;

		if (log->type == PURPLE_LOG_CHAT) {
			chat = g_strdup_printf("%s.chat", guy);
			g_free(guy);
			guy = chat;
		}
		logfile = g_strdup_printf("%s.log", guy);
		g_free(guy);

		dir = g_build_filename(ud, "logs", NULL);
		purple_build_dir (dir, S_IRUSR | S_IWUSR | S_IXUSR);

		filename = g_build_filename(dir, logfile, NULL);
		g_free(dir);
		g_free(logfile);

		log->logger_data = data = g_new0(struct basic_logger_data, 1);

		if (g_stat(filename, &st) < 0)
			data->new = TRUE;
		else
			data->old_last_modified = st.st_mtime;

		data->file = g_fopen(filename, "a");
		if (!data->file) {
			purple_debug(PURPLE_DEBUG_ERROR, "log", "Could not create log file %s\n", filename);
			g_free(filename);
			g_free(data);
			return_written;
		}
		data->filename = filename;

		if (data->new)
			written += fprintf(data->file, _("IM Sessions with %s\n"), purple_normalize(log->account, log->name));

		written += fprintf(data->file, "---- New Conversation @ %s ----\n",
			oldlogger_date_full());
		data->offset = ftell(data->file);
	}

	/* if we can't write to the file, give up before we hurt ourselves */
	if(!data->file)
		return_written;

	purple_markup_html_to_xhtml(message, NULL, &stripped);

	if(log->type == PURPLE_LOG_SYSTEM){
		if (!strncmp(stripped, "+++ ", 4)) {
			written += fprintf(data->file, "---- %s @ %s ----\n", stripped, oldlogger_date_full());
		} else {
			written += fprintf(data->file, "---- %s (%s) reported that %s @ %s ----\n", purple_account_get_username(log->account), prpl, stripped, oldlogger_date_full());
		}
	} else {
		strftime(date, sizeof(date), "%H:%M:%S", localtime(&time));
		if (type & PURPLE_MESSAGE_SEND ||
			type & PURPLE_MESSAGE_RECV) {
			if (type & PURPLE_MESSAGE_AUTO_RESP) {
				written += fprintf(data->file, _("(%s) %s <AUTO-REPLY>: %s\n"), date,
						from, stripped);
			} else {
				if(purple_message_meify(stripped, -1))
					written += fprintf(data->file, "(%s) ***%s %s\n", date, from,
							stripped);
				else
					written += fprintf(data->file, "(%s) %s: %s\n", date, from,
							stripped);
			}
		} else if (type & PURPLE_MESSAGE_SYSTEM)
			written += fprintf(data->file, "(%s) %s\n", date, stripped);
		else if (type & PURPLE_MESSAGE_NO_LOG) {
			/* This shouldn't happen */
			g_free(stripped);
			return_written;
		} else if (type & PURPLE_MESSAGE_WHISPER)
			written += fprintf(data->file, "(%s) *%s* %s\n", date, from, stripped);
		else
			written += fprintf(data->file, "(%s) %s%s %s\n", date, from ? from : "",
					from ? ":" : "", stripped);
	}

	fflush(data->file);
	g_free(stripped);

	return_written;
}

#if !PURPLE_VERSION_CHECK(2,0,0)
static PurpleLogLogger oldtxt_logger = {
	N_("Old plain text"), "oldtxt",
	old_logger_create,
	oldtxt_logger_write,
	old_logger_finalize,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};
#endif


/****************************
 ** Ye Olde HTML LOGGER *****
 ****************************/
#if PURPLE_VERSION_CHECK(2,0,0)
static gsize
#else
static void
#endif
oldhtml_logger_write(PurpleLog *log, PurpleMessageFlags type,
					const char *from, time_t time, const char *message)
{
	char date[64];
	char *msg_fixed = NULL;
	struct basic_logger_data *data = log->logger_data;
	const char *prpl = (purple_find_prpl(purple_account_get_protocol_id(log->account)))->info->name;
	gsize written = 0;
	if(!data) {
		/* This log is new */
		const char *ud = purple_user_dir();
		char *filename;
		char *guy = g_strdup(purple_normalize(log->account, log->name));
		char *chat;
		char *dir;
		char *logfile;
		struct stat st;

		if (log->type == PURPLE_LOG_CHAT) {
			chat = g_strdup_printf("%s.chat", guy);
			g_free(guy);
			guy = chat;
		}
		logfile = g_strdup_printf("%s.log", guy);
		g_free(guy);

		dir = g_build_filename(ud, "logs", NULL);
		purple_build_dir (dir, S_IRUSR | S_IWUSR | S_IXUSR);

		filename = g_build_filename(dir, logfile, NULL);
		g_free(dir);
		g_free(logfile);

		log->logger_data = data = g_new0(struct basic_logger_data, 1);

		if (g_stat(filename, &st) < 0)
			data->new = TRUE;
		else
			data->old_last_modified = st.st_mtime;

		data->file = g_fopen(filename, "a");
		if (!data->file) {
			purple_debug(PURPLE_DEBUG_ERROR, "log",
					"Could not create log file %s\n", filename);
			g_free(filename);
			g_free(data);
			return_written;
		}
		data->filename = filename;

		if (data->new) {
			written += fprintf(data->file, "<HTML><HEAD><TITLE>");
			written += fprintf(data->file, _("IM Sessions with %s"), purple_normalize(log->account, log->name));
			written += fprintf(data->file, "</TITLE></HEAD><BODY BGCOLOR=\"#ffffff\">\n");
		}
		written += fprintf(data->file, "<HR><BR><H3 Align=Center> ");
		written += fprintf(data->file, "---- New Conversation @ %s ----</H3><BR>\n",
			oldlogger_date_full());
		data->offset = ftell(data->file);
	}

	/* if we can't write to the file, give up before we hurt ourselves */
	if(!data->file)
		return_written;

	purple_markup_html_to_xhtml(message, &msg_fixed, NULL);

	if(log->type == PURPLE_LOG_SYSTEM){
		if (!strncmp(msg_fixed, "+++ ", 4)) {
			written += fprintf(data->file, "---- %s @ %s ----<BR>\n", msg_fixed, oldlogger_date_full());
		} else {
			written += fprintf(data->file, "---- %s (%s) reported that %s @ %s ----<BR>\n", purple_account_get_username(log->account), prpl, msg_fixed, oldlogger_date_full());
		}
	} else {
		strftime(date, sizeof(date), "%H:%M:%S", localtime(&time));
		if (type & PURPLE_MESSAGE_SYSTEM)
			written += fprintf(data->file, "<FONT COLOR=\"#000000\" sml=\"%s\">(%s) <B>%s</B></FONT><BR>\n", prpl, date, msg_fixed);
		else if (type & PURPLE_MESSAGE_WHISPER)
			written += fprintf(data->file, "<FONT COLOR=\"#6C2585\" sml=\"%s\">(%s) <B>%s:</B></FONT> %s<BR>\n",
					prpl, date, from, msg_fixed);
		else if (type & PURPLE_MESSAGE_AUTO_RESP) {
			if (type & PURPLE_MESSAGE_SEND)
				written += fprintf(data->file, _("<FONT COLOR=\"#16569E\" sml=\"%s\">(%s) <B>%s &lt;AUTO-REPLY&gt;:</B></FONT> %s<BR>\n"), prpl, date, from, msg_fixed);
			else if (type & PURPLE_MESSAGE_RECV)
				written += fprintf(data->file, _("<FONT COLOR=\"#A82F2F\" sml=\"%s\">(%s) <B>%s &lt;AUTO-REPLY&gt;:</B></FONT> %s<BR>\n"), prpl, date, from, msg_fixed);
		} else if (type & PURPLE_MESSAGE_RECV) {
			if(purple_message_meify(msg_fixed, -1))
				written += fprintf(data->file, "<FONT COLOR=\"#6C2585\" sml=\"%s\">(%s) <B>***%s</B></FONT> <font sml=\"%s\">%s</FONT><BR>\n",
						prpl, date, from, prpl, msg_fixed);
			else
				written += fprintf(data->file, "<FONT COLOR=\"#A82F2F\" sml=\"%s\">(%s) <B>%s:</B></FONT> <font sml=\"%s\">%s</FONT><BR>\n",
						prpl, date, from, prpl, msg_fixed);
		} else if (type & PURPLE_MESSAGE_SEND) {
			if(purple_message_meify(msg_fixed, -1))
				written += fprintf(data->file, "<FONT COLOR=\"#6C2585\" sml=\"%s\">(%s) <B>***%s</B></FONT> <font sml=\"%s\">%s</FONT><BR>\n",
						prpl, date, from, prpl, msg_fixed);
			else
				written += fprintf(data->file, "<FONT COLOR=\"#16569E\" sml=\"%s\">(%s) <B>%s:</B></FONT> <font sml=\"%s\">%s</FONT><BR>\n",
						prpl, date, from, prpl, msg_fixed);
		}
	}

	fflush(data->file);
	g_free(msg_fixed);

	return_written;
}

#if !PURPLE_VERSION_CHECK(2,0,0)
static PurpleLogLogger oldhtml_logger = {
	N_("Old HTML"), "oldhtml",
	old_logger_create,
	oldhtml_logger_write,
	old_logger_finalize,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};
#endif

static gboolean
plugin_load(PurplePlugin *plugin)
{
#if PURPLE_VERSION_CHECK(2,0,0)
	oldtxt_logger = purple_log_logger_new("oldtxt", N_("Old plain text"), 3,
										old_logger_create,
										oldtxt_logger_write,
										old_logger_finalize);
	purple_log_logger_add(oldtxt_logger);
	oldhtml_logger = purple_log_logger_new("oldhtml", N_("Old HTML"), 3,
										 old_logger_create,
										 oldhtml_logger_write,
										 old_logger_finalize);
	purple_log_logger_add(oldhtml_logger);
#else
	purple_log_logger_add(&oldtxt_logger);
	purple_log_logger_add(&oldhtml_logger);
#endif
	purple_prefs_trigger_callback("/purple/logging/format");
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
#if PURPLE_VERSION_CHECK(2,0,0)
	purple_log_logger_remove(oldtxt_logger);
	purple_log_logger_remove(oldhtml_logger);
#else
	purple_log_logger_remove(&oldtxt_logger);
	purple_log_logger_remove(&oldhtml_logger);
#endif
	purple_prefs_trigger_callback("/purple/logging/format");
	return TRUE;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,								/**< magic			*/
	PURPLE_MAJOR_VERSION,								/**< major version	*/
	PURPLE_MINOR_VERSION,								/**< minor version	*/
	PURPLE_PLUGIN_STANDARD,							/**< type			*/
	NULL,											/**< ui_requirement	*/
	0,												/**< flags			*/
	NULL,											/**< dependencies	*/
	PURPLE_PRIORITY_DEFAULT,							/**< priority		*/

	OLDLOGGER_PLUGIN_ID,							/**< id				*/
	NULL,											/**< name			*/
	PP_VERSION,									/**< version		*/
	NULL,											/**< summary		*/
	NULL,											/**< description	*/
	"Stu Tomlinson <stu@nosnilmot.com>",			/**< author			*/
	PP_WEBSITE,									/**< homepage		*/

	plugin_load,									/**< load			*/
	plugin_unload,									/**< unload			*/
	NULL,											/**< destroy		*/

	NULL,											/**< ui_info		*/
	NULL,											/**< extra_info		*/
	NULL,											/**< prefs_info		*/
	NULL											/**< actions		*/
};

static void
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Old Logger");
	info.summary = _("Re-implements the legacy, deficient, logging");
	info.description = _("Re-implements the legacy, deficient, logging");
}

PURPLE_INIT_PLUGIN(oldlogger, init_plugin, info)

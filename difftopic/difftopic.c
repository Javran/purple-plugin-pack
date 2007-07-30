/*
 * DiffTopic - Show the old topic when the topic in a chat room changes.
 * Copyright (C) 2006
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
#ifdef HAVE_CONFIG_H
# include "../pp_config.h"
#endif

#define PURPLE_PLUGINS

#define PLUGIN_ID			"gtk-plugin_pack-difftopic"
#define PLUGIN_NAME			"DiffTopic"
#define PLUGIN_STATIC_NAME	"DiffTopic"
#define PLUGIN_SUMMARY		"Show the old topic when the topic in a chat room changes."
#define PLUGIN_DESCRIPTION	"Show the old topic when the topic in a chat room changes."
#define PLUGIN_AUTHOR		"Sadrul H Chowdhury <sadrul@users.sourceforge.net>"

/* System headers */
#include <glib.h>
#include <ctype.h>
#include <string.h>

/* Pack/Local headers */
#include "../common/pp_internal.h"

/* Purple headers */
#include <plugin.h>
#include <version.h>
#include <gtkplugin.h>

#include <conversation.h>

#include <gtkconv.h>
#include <gtkimhtml.h>

#define SAME(a,b)  ((isalnum((a)) && isalnum((b))) || (!isalnum((a)) && !isalnum((b))))

#define SIZE 1000   /* XXX: Let's assume this always holds */
static int lcs[SIZE][SIZE];

static GList *
split_string(const char *string)
{
	GList *ret = NULL;
	const char *start, *end;

	start = end = string;
	while (*start) {
		while (*end && SAME(*start, *end)) {
			end++;
		}
		ret = g_list_append(ret, g_strndup(start, end - start));
		start = end;
	}

	return ret;
}

static GString *
g_string_prepend_printf(GString *string, const char *format, ...)
{
	va_list args;
	char *str;

	va_start(args, format);
	str = g_strdup_vprintf(format, args);
	va_end(args);
	string = g_string_prepend(string, str);
	g_free(str);
	return string;
}

/* Let's LCS */
static void
have_fun(GtkIMHtml *imhtml, const char *old, const char *new)
{
	GList *lold, *lnew;
	int i, j, m, n;
	GString *from, *to;
	char *text;

	lold = split_string(old);
	lnew = split_string(new);

	memset(lcs, 0, sizeof(lcs));
	n = g_list_length(lold);
	m = g_list_length(lnew);

	for (i = 1; i <= n; i++)
		lcs[i][0] = 0;

	for (j = 1; j <= m; j++)
		lcs[0][j] = 0;

	for (i = 1; i <= n; i++) {
		for (j = 1; j <= m; j++) {
			if (strcmp(g_list_nth_data(lold, i-1), g_list_nth_data(lnew, j-1)) == 0)
				lcs[i][j] = lcs[i-1][j-1] + 1;
			else
				lcs[i][j] = (lcs[i-1][j] >= lcs[i][j-1] ? lcs[i-1][j] : lcs[i][j-1]);
		}
	}

	from = g_string_new(NULL);
	to = g_string_new(NULL);
	i = n;
	j = m;
	while (i && j) {
		if (lcs[i][j] == lcs[i-1][j]) {
			from = g_string_prepend_printf(from, "<font color='red'><B>%s</B></font>",
						g_list_nth_data(lold, i - 1));
			i--;
		} else if (lcs[i][j] == lcs[i][j-1]) {
			to = g_string_prepend_printf(to, "<font color='green'><B>%s</B></font>",
						g_list_nth_data(lnew, j - 1));
			j--;
		} else if (lcs[i][j] == lcs[i-1][j-1] + 1) {
			from = g_string_prepend_printf(from, "<B>%s</B>", g_list_nth_data(lold, i-1));
			to = g_string_prepend_printf(to, "<B>%s</B>", g_list_nth_data(lnew, j-1));
			i--;
			j--;
		}
	}

	while (j) {
		to = g_string_prepend_printf(to, "<font color='green'><B>%s</B></font>",
					g_list_nth_data(lnew, j - 1));
		j--;
	}

	while (i) {
		from = g_string_prepend_printf(from, "<font color='red'><B>%s</B></font>",
					g_list_nth_data(lold, i - 1));
		i--;
	}

	text = g_strdup_printf(_("<BR>Topic changed from: <BR>%s<BR>To:<BR>%s"), from->str, to->str);
	gtk_imhtml_append_text(GTK_IMHTML(imhtml), text, 0);
	g_free(text);

	g_string_free(from, TRUE);
	g_string_free(to, TRUE);
	g_list_foreach(lold, (GFunc)g_free, NULL);
	g_list_foreach(lnew, (GFunc)g_free, NULL);
	g_list_free(lold);
	g_list_free(lnew);
}

static void
topic_changed(PurpleConversation *conv, const char *who, const char *what)
{
	PidginConversation *gtkconv = conv->ui_data;
	char *old;

	old = g_object_get_data(G_OBJECT(gtkconv->imhtml), "difftopic");
	if (old && what) {
		have_fun(GTK_IMHTML(gtkconv->imhtml), old, what);
	}
	g_object_set_data_full(G_OBJECT(gtkconv->imhtml), "difftopic", g_strdup(what), (GDestroyNotify)g_free);
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	purple_signal_connect(purple_conversations_get_handle(), "chat-topic-changed",
				plugin, PURPLE_CALLBACK(topic_changed), NULL);
	/* Set the topic to the opened chat windows */
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	return TRUE;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,    /* Magic              */
	PURPLE_MAJOR_VERSION,   /* Purple Major Version */
	PURPLE_MINOR_VERSION,   /* Purple Minor Version */
	PURPLE_PLUGIN_STANDARD, /* plugin type        */
	PIDGIN_PLUGIN_TYPE, /* ui requirement     */
	0,                    /* flags              */
	NULL,                 /* dependencies       */
	PURPLE_PRIORITY_DEFAULT,/* priority           */

	PLUGIN_ID,            /* plugin id          */
	NULL,                 /* name               */
	PP_VERSION,          /* version            */
	NULL,                 /* summary            */
	NULL,                 /* description        */
	PLUGIN_AUTHOR,        /* author             */
	PP_WEBSITE,          /* website            */

	plugin_load,          /* load               */
	plugin_unload,        /* unload             */
	NULL,                 /* destroy            */

	NULL,                 /* ui_info            */
	NULL,                 /* extra_info         */
	NULL,                 /* prefs_info         */
	NULL,                  /* actions            */
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _(PLUGIN_NAME);
	info.summary = _(PLUGIN_SUMMARY);
	info.description = _(PLUGIN_DESCRIPTION);
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info)

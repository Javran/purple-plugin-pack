/*
 * A completely stupid plugin, inspired by a dumb conversation in #gaim
 * and needing some light relief from 'real' work.
 * Also as a tribute to our fearless project leader.
 * Copyright (C) 2005 Peter Lawler <bleeter from users.sf.net>
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

/*
 * 08:14 < OgreBoy> well, gaim isn't a gnome app, so it shouldn't be in /opt/gnome anyway
 * 08:17 < flurble> not a gnome app? but it begins with g!
 * 08:18 < Bleeter> Google is a gnome app!
 * 08:19 < grim> i was going to use my nick as an example but decided against that..
*/

#ifdef HAVE_CONFIG_H
# include "../pp_config.h"
#endif /* HAVE_CONFIG_H */

#define PURPLE_PLUGINS

#include <glib.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "../common/pp_internal.h"

#include <gtkconv.h>
#include <gtkimhtml.h>
#include <gtkplugin.h>

#include <cmds.h>
#include <conversation.h>
#include <debug.h>
#include <plugin.h>
#include <version.h>

#if GLIB_CHECK_VERSION(2,6,0)
#include <glib/gstdio.h>
#else
#include <sys/stat.h>
#define g_fopen fopen
#define g_rename rename
#define g_stat stat
#define g_unlink unlink
#endif

#define	MAX_LENGTH	1024

#define COMPOSER "Howard Goodall"
#define LYRICISTS "Kim Fuller and Doug Naylor"

const char *LYRICS[] = {
"If you're in trouble he will save the day",
"He's brave and he's fearless come what may",
"Without him the mission would go astray",
"",
"He's Arnold, Arnold, Arnold Rimmer",
"Without him life would be much grimmer",
"He's handsome, trim and no-one slimmer",
"He will never need a Zimmer",
"",
"Ask Arnold, Arnold, Arnold Rimmer",
"More reliable than a garden trimmer",
"He's never been mistaken for Yul Brynner",
"He's not bald and his head doesn't glimmer",
"",
"Master of the wit and the repartee",
"His command of space directives is uncanny",
"How come he's such a genius? Don't ask me",
"",
"Ask Arnold, Arnold, Arnold Rimmer",
"He's also a fantastic swimmer",
"And if you play your cards right",
"Then he just might come around for dinner",
"",
"He's Arnold, Arnold, Arnold Rimmer",
"No rhymes left now apart from quimmer",
"I hope they fade us out before we get to shlimmer",
"Fade out you stupid brimmer",
NULL
};

struct lyrics_and_info {
	GList *lyric; 		/* The line of lyric */
	gboolean verse; 	/* TRUE for Verse, FALSE for Chorus */
	guint time; 		/* Time period the lyric lasts for, in milliseconds */
	guint gap; 			/* Gap between last lyric and the next, in milliseconds */
};

struct timeout_data
{
	struct lyrics_and_info *info;
	PurpleConversation *conv;
};

static PurpleCmdId rim_cmd_id = 0;

static gboolean
timeout_func_cb(struct timeout_data *data)
{
	char *msg;
	const char *color;
	PidginConversation *gtkconv = PIDGIN_CONVERSATION(data->conv);
	GtkIMHtml *imhtml = GTK_IMHTML(gtkconv->entry);
	GList *list;

	if (data->info->lyric == NULL) {
		/* XXX: free the lyric if it was dyn-allocated */
		g_free(data->info);
		g_free(data);
		return FALSE;
	}

	color = imhtml->edit.forecolor;

	list = data->info->lyric;

	if (list->next == NULL) {
		/* Is this Ugly or is this UGLY? */
		int len = strlen(list->data);
		GdkColor gdkcolor;
		int inc_r, inc_g, inc_b;
		char *s = list->data;

		if (!gdk_color_parse(color, &gdkcolor))
			gdkcolor.red = gdkcolor.green = gdkcolor.blue = 0;

		inc_r = (255 - (gdkcolor.red >> 8))/len;
		inc_g = (255 - (gdkcolor.green >> 8))/len;
		inc_b = (255 - (gdkcolor.blue >> 8))/len;

		msg = g_strdup("");
		while(*s) {
			char *temp = msg;
			char t[2] = {*s++, 0};
			char tag[16];

			snprintf(tag, sizeof(tag), "#%02x%02x%02x",
					gdkcolor.red >> 8, gdkcolor.green >> 8, gdkcolor.blue >> 8);

			msg = g_strconcat(msg, "<font color=\"", tag, "\">", t, "</font>", NULL);
			g_free(temp);

			gdkcolor.red += inc_r << 8;
			gdkcolor.green += inc_g << 8;
			gdkcolor.blue += inc_b << 8;
		}
	} else if (color)
		msg = g_strdup_printf("<font color=\"%s\">%s</font>", color, (char *)list->data);
	else
		msg = g_strdup(*(char*)list->data ? (char*)list->data : "&nbsp;");

	if (purple_conversation_get_type(data->conv) == PURPLE_CONV_TYPE_IM)
		purple_conv_im_send(PURPLE_CONV_IM(data->conv), msg);
	else if (purple_conversation_get_type(data->conv) == PURPLE_CONV_TYPE_CHAT)
		purple_conv_chat_send(PURPLE_CONV_CHAT(data->conv), msg);
	g_free(msg);

	g_free(list->data);
	data->info->lyric = list->next;
	list->next = NULL;
	g_list_free(list);		/* XXX: perhaps just a g_free? */

	/* what stuff do you do about the verse? */

	return TRUE;
}

static GList *
rim_get_file_lines(const char *filename)
{
	GList *list = NULL;
	FILE *file = NULL;
	char str[MAX_LENGTH];

	file = g_fopen(filename, "r");

	if (!file)				/* XXX: Show an error message that the file doesn't exist */
		return NULL;

	while (fgets(str, MAX_LENGTH, file)) {
		char *s = str + strlen(str) - 1;
		if (*s == '\r' || *s == '\n')
			*s = 0;
		list = g_list_append(list, g_strdup(str));
	}

	fclose(file);

	return list;
}

static PurpleCmdRet
rim(PurpleConversation *conv, const gchar *cmd, gchar **args,
	 gchar *error, void *null)
{
	struct timeout_data *data = g_new0(struct timeout_data, 1);
	struct lyrics_and_info *info = g_new0(struct lyrics_and_info, 1);
	PidginConversation *gtkconv = PIDGIN_CONVERSATION(conv);
	gint source;

	/* XXX: Need to manually parse the arguments :-/ */
	if (*args && *(args+1)) {
		/* two parameters: filename duration (in seconds) */
		info->lyric = rim_get_file_lines(*(args + 1));
		sscanf(*args, "%d", &info->time);
		info->time *= 1000;
	} else if(*args) {
		if(!g_ascii_strcasecmp(*args, "quit")) {
			GList *list = NULL;
			list = g_list_append(list, "Fine, I'll stop");
			g_list_foreach(info->lyric, (GFunc)g_free, NULL);
			g_list_free(info->lyric);
			info->lyric = list;
			info->verse = FALSE;
			info->time = 5000;
		}
		else {
			g_list_free(info->lyric);
			info->lyric = NULL;
		}
	} else {
		int i = 0;
		GList *list = NULL;

		while (LYRICS[i]) {
			list = g_list_append(list, g_strdup(LYRICS[i]));
			i++;
		}

		info->lyric = list;
		info->verse = TRUE;
		info->time = 60000;
	}

	purple_debug_info("gRIM", "HINT: quit with quit\n");

	if (info->lyric == NULL) {
		g_free(info);
		g_free(data);
		return PURPLE_CMD_STATUS_FAILED;
	}

	info->gap = info->time / g_list_length(info->lyric);
	if (info->gap < 5000)
		info->gap = 5000;

	data->info = info;
	data->conv = conv;

	/* XXX: make sure we stop when conv-is destroyed. */

	source = g_timeout_add(info->gap, (GSourceFunc)timeout_func_cb, data);
	g_object_set_data_full(G_OBJECT(gtkconv->imhtml), "gRim:timer",
			GINT_TO_POINTER(source), (GDestroyNotify)g_source_remove);

	return PURPLE_CMD_RET_OK;
}

static gboolean
plugin_load(PurplePlugin *plugin) {
	const gchar *help;

	/* should be completely mad and see if user has only one buddy (not a chat)
	 *  on the blist and pluralise if appropriate */
	help = _("gRIM: rim your pals\n"
			"/rim &lt;duration-in-secs&gt; &lt;filename&gt;");

	rim_cmd_id = purple_cmd_register("rim", "ws", PURPLE_CMD_P_PLUGIN,
									PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS,
									NULL, PURPLE_CMD_FUNC(rim),
									help, NULL);

	/* THIS LINE IS NOT TRANSLATABLE. Patches to make it NLS capable will be
	 * rejected without response */
	help = "gRIM: Take off every 'Zig'!!";
	rim_cmd_id = purple_cmd_register("base", "", PURPLE_CMD_P_PLUGIN,
									PURPLE_CMD_FLAG_IM | PURPLE_CMD_FLAG_CHAT,
									NULL, PURPLE_CMD_FUNC(rim),
									help, NULL);
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin) {
	purple_cmd_unregister(rim_cmd_id);

	return TRUE;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,								/**< magic			*/
	PURPLE_MAJOR_VERSION,								/**< major version	*/
	PURPLE_MINOR_VERSION,								/**< minor version	*/
	PURPLE_PLUGIN_STANDARD,								/**< type			*/
	PIDGIN_PLUGIN_TYPE,									/**< ui_requirement	*/
	0,													/**< flags			*/
	NULL,												/**< dependencies	*/
	PURPLE_PRIORITY_DEFAULT,							/**< priority		*/

	"gtk-plugin_pack-gRIM",								/**< id				*/
	NULL,												/**< name			*/
	PP_VERSION,											/**< version		*/
	NULL,												/**  summary		*/
	NULL,												/**  description	*/
	"Peter Lawler <bleeter from users.sf.net> and"
	" Sadrul Habib Chowdhury <sadrul from users.sf.net>",
														/**< authors		*/
	PP_WEBSITE,											/**< homepage		*/

	plugin_load,										/**< load			*/
	plugin_unload,										/**< unload			*/
	NULL,												/**< destroy		*/

	NULL,												/**< ui_info		*/
	NULL,												/**< extra_info		*/
	NULL,												/**< prefs_info		*/
	NULL,												/**< actions		*/
	NULL,												/**< reserved 1		*/
	NULL,												/**< reserved 2		*/
	NULL,												/**< reserved 3		*/
	NULL												/**< reserved 4		*/
};

static void
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif
	info.name = _("gRIM");
	info.summary = _("A completely stupid and pointless plugin");
	info.description = _("Adds commands to annoy buddies with. Inspired by a dumb IRC convo and Red Dwarf.");
}

/*
grim suggested I should /base this plugin. By adding this in, I should get the
guts of this thing able to handle different text inputs, so I thought it was
a good thing to do.

For the moment, I've thrown in the skeleton command registration, and the text
here which I got from wikipedia.

    Narrator: In A.D. 2101, war was beginning.

    Captain: What happen ?
    Mechanic: Somebody set up us the bomb.

    Operator: We get signal.
    Captain: What !
    Operator: Main screen turn on.
    Captain: It's you !!
    CATS: How are you gentlemen !!
    CATS: All your base are belong to us.
    CATS: You are on the way to destruction.
    Captain: What you say !!
    CATS: You have no chance to survive make your time.
    CATS: Ha Ha Ha Ha ....

    Operator: Captain !!
    Captain: Take off every 'Zig'!!
    Captain: You know what you doing.
    Captain: Move 'Zig'.
    Captain: For great justice.
*/

PURPLE_INIT_PLUGIN(flip, init_plugin, info)

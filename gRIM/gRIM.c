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

#if HAVE_CONFIG_H
# include "../gpp_config.h"
#endif /* HAVE_CONFIG_H */

#define GAIM_PLUGINS

#include <gtkconv.h>
#include <gtkimhtml.h>
#include <gtkplugin.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include <cmds.h>
#include <conversation.h>
#include <debug.h>
#include <plugin.h>
#include <version.h>

#include "../common/i18n.h"

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
	GaimConversation *conv;
};

static GaimCmdId rim_cmd_id = 0;

static gboolean
timeout_func_cb(struct timeout_data *data)
{
	char *msg;
	const char *color;
	GaimGtkConversation *gtkconv = GAIM_GTK_CONVERSATION(data->conv);
	GtkIMHtml *imhtml = GTK_IMHTML(gtkconv->entry);
	GList *list;

	if (data->info->lyric == NULL)
	{
		/* XXX: free the lyric if it was dyn-allocated */
		g_free(data->info);
		g_free(data);
		return FALSE;
	}

	color = imhtml->edit.forecolor;

	list = data->info->lyric;

	if (list->next == NULL)
	{
		/* Is this Ugly or is this UGLY? */
		int len = strlen(list->data);
		GdkColor gdkcolor;
		int inc_r, inc_g, inc_b;
		char *s = list->data;

		if (!gdk_color_parse(color, &gdkcolor))
		{
			gdkcolor.red = gdkcolor.green = gdkcolor.blue = 0;
		}

		inc_r = (255 - (gdkcolor.red >> 8))/len;
		inc_g = (255 - (gdkcolor.green >> 8))/len;
		inc_b = (255 - (gdkcolor.blue >> 8))/len;

		msg = g_strdup("");
		while(*s)
		{
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
	}
	else if (color)
		msg = g_strdup_printf("<font color=\"%s\">%s</font>", color, (char *)list->data);
	else
		msg = g_strdup(*(char*)list->data ? (char*)list->data : "&nbsp;");

	if (gaim_conversation_get_type(data->conv) == GAIM_CONV_TYPE_IM)
		gaim_conv_im_send(GAIM_CONV_IM(data->conv), msg);
	else if (gaim_conversation_get_type(data->conv) == GAIM_CONV_TYPE_CHAT)
		gaim_conv_chat_send(GAIM_CONV_CHAT(data->conv), msg);
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

	while (fgets(str, MAX_LENGTH, file))
	{
		char *s = str + strlen(str) - 1;
		if (*s == '\r' || *s == '\n')
			*s = 0;
		list = g_list_append(list, g_strdup(str));
	}

	fclose(file);

	return list;
}

static GaimCmdRet
rim(GaimConversation *conv, const gchar *cmd, gchar **args,
	 gchar *error, void *null)
{
	struct timeout_data *data = g_new0(struct timeout_data, 1);
	struct lyrics_and_info *info = g_new0(struct lyrics_and_info, 1);
	GaimGtkConversation *gtkconv = GAIM_GTK_CONVERSATION(conv);
	gint source;

	/* XXX: Need to manually parse the arguments :-/ */
	if (*args && *(args+1))
	{
		/* two parameters: filename duration (in seconds) */
		info->lyric = rim_get_file_lines(*args);
		sscanf(*(args+1), "%d", &info->time);
		info->time *= 1000;
	}
	else if (*args)
	{
		/* one parameter: filename */
		info->lyric = rim_get_file_lines(*args);
		info->time = g_list_length(info->lyric) * 5000;	/* at least 5 seconds between two lines */
	}
	else
	{
		int i = 0;
		GList *list = NULL;
		while (LYRICS[i])
		{
			list = g_list_append(list, g_strdup(LYRICS[i]));
			i++;
		}
		info->lyric = list;
		info->verse = TRUE;
		info->time = 60000;
	}

	if (info->lyric == NULL)
	{
		g_free(info);
		g_free(data);
		return GAIM_CMD_STATUS_FAILED;
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

	return GAIM_CMD_RET_OK;
}

static gboolean
plugin_load(GaimPlugin *plugin) {
	const gchar *help;

	/* should be completely mad and see if user has only one buddy (not a chat)
	 *  on the blist and pluralise if appropriate */
	help = _("gRIM: rim your pals\n"
			"/rim &lt;filename&gt; &lt;duration-in-secs&gt;");

	rim_cmd_id = gaim_cmd_register("rim", "ws", GAIM_CMD_P_PLUGIN,
									GAIM_CMD_FLAG_IM | GAIM_CMD_FLAG_CHAT,
									NULL, GAIM_CMD_FUNC(rim),
									help, NULL);

	/* THIS LINE IS NOT TRANSLATABLE. Patches to make it NLS capable will be
	 * rejected without response */
	help = "gRIM: Take off every 'Zig'!!";
	rim_cmd_id = gaim_cmd_register("base", "", GAIM_CMD_P_PLUGIN,
									GAIM_CMD_FLAG_IM | GAIM_CMD_FLAG_CHAT,
									NULL, GAIM_CMD_FUNC(rim),
									help, NULL);
	return TRUE;
}

static gboolean
plugin_unload(GaimPlugin *plugin) {
	gaim_cmd_unregister(rim_cmd_id);

	return TRUE;
}

static GaimPluginInfo info =
{
	GAIM_PLUGIN_MAGIC,								/**< magic			*/
	GAIM_MAJOR_VERSION,								/**< major version	*/
	GAIM_MINOR_VERSION,								/**< minor version	*/
	GAIM_PLUGIN_STANDARD,							/**< type			*/
	GAIM_GTK_PLUGIN_TYPE,							/**< ui_requirement	*/
	0,												/**< flags			*/
	NULL,											/**< dependencies	*/
	GAIM_PRIORITY_DEFAULT,							/**< priority		*/

	"gtk-plugin_pack-gRIM",							/**< id				*/
	NULL,											/**< name			*/
	GPP_VERSION,									/**< version		*/
	NULL,											/**  summary		*/
	NULL,											/**  description	*/
	"Peter Lawler <bleeter from users.sf.net> and"
	" Sadrul Habib Chowdhury <sadrul from users.sf.net>",
													/**< authors		*/
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

GAIM_INIT_PLUGIN(flip, init_plugin, info)

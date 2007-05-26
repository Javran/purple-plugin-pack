/*
  Purple-Plonkers - Manager the plonkers out in cyberland
  Copyright (C) 2005 Peter Lawler

  Very loosely based on gxr, Copyright (C) 2004 Gary Kramlich

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#ifdef HAVE_CONFIG_H
# include "../pp_config.h"
#endif

#define PURPLE_PLUGINS

#include <cmds.h>
#include <conversation.h>
#include <debug.h>
#include <gtkconv.h>
#include <gtkplugin.h>
#include <gtkprefs.h>
#include <gtkutils.h>
#include <plugin.h>
#include <prefs.h>
#include <util.h>
#include <version.h>

/* #define PLONKERS_DEBUG */
/*******************************************************************************
 * Constants
 ******************************************************************************/

/*******************************************************************************
 * Globals
 ******************************************************************************/
static PurpleCmdId plonkers_cmd;
static PurpleCmdId plonk_cmd;

/*******************************************************************************
 * Callbacks
 ******************************************************************************/

/*******************************************************************************
 * Helpers
 ******************************************************************************/
static gchar *
plonkers_format_info(PurpleConversation *conv) {
	GString *plonkers_str;
	gchar *ret, *plonkers_char;
	const gchar *format;
	GList *plonkers_list;
	guint plonkers_size;

	plonkers_list = purple_conv_chat_get_ignored(PURPLE_CONV_CHAT(conv));
	if (!plonkers_list)
		return NULL;
	plonkers_size = g_list_length (plonkers_list);
	plonkers_char = NULL;
	for ( ; plonkers_list; plonkers_list = plonkers_list->next) {
		if (!plonkers_char) {
			plonkers_char = g_strdup_printf("%s", (char *)plonkers_list->data);
		} else {
			plonkers_char = g_strdup_printf("%s, %s", plonkers_char, (char *)plonkers_list->data);
		}
	}
	plonkers_str = g_string_new("");
	if (plonkers_size == 1) {
		format = g_strdup(purple_prefs_get_string("/plugins/core/plugin_pack/plonkers/plonkers/format_singular"));
	} else {
		format = g_strdup(purple_prefs_get_string("/plugins/core/plugin_pack/plonkers/plonkers/format_plural"));
	}

	while(format) {
#ifdef PLONKERS_DEBUG
		purple_debug_info("plonkers", "Str: %s\n", plonkers_str->str);
#endif
		if(format[0] != '%') {
			plonkers_str = g_string_append_c(plonkers_str, format[0]);
			format++;
			continue;
		}

		format++;
		if(!format[0])
			break;

		switch(format[0]) {
			case '%':
				plonkers_str = g_string_append_c(plonkers_str, '%');
				break;
			case 'N':
				g_string_append_printf(plonkers_str, "%d", plonkers_size);
				break;
			case 'P':
				plonkers_str = g_string_append(plonkers_str, plonkers_char);
				break;
		}
		format++;
	}
	ret = plonkers_str->str;
	g_string_free(plonkers_str, FALSE);
	if (plonkers_char)
		g_free(plonkers_char);
	purple_debug_info("plonkers", "Formatted plonkers: %s\n", ret);
	return ret;
}

static void
plonkers_display(PurpleConversation *conv) {
	gchar *text = NULL;

	g_return_if_fail(conv);
	text = plonkers_format_info(conv);

	if(!text)
		return;
	purple_conv_chat_send(PURPLE_CONV_CHAT(conv), text);
	if(text)
		g_free(text);
}

/*******************************************************************************
 * Command cb's
 ******************************************************************************/
static PurpleCmdRet
plonkers_cmd_cb(PurpleConversation *c, const gchar *cmd, gchar **args, gchar **error, void *data) {
 /* I plan a switch that dumps the current 'block' list, once purple privacy
  * can export */
#if 0
	gchar *lower;

	if (args[0])
		lower = g_ascii_strdown(args[0], strlen(args[0]));
#endif
	plonkers_display(c);
#if 0
	if (args[0])
		g_free(lower);
#endif
	return PURPLE_CMD_RET_OK;
}

static PurpleCmdRet
plonk_cmd_cb(PurpleConversation *c, const gchar *cmd, gchar **args, gchar **error, void *data) {
/* this is the funky 'mass block/ignore' routine.
 * given a/n list of ID/'s it'll add that|those to all block|ignore lists
 * of each account of the same prpl type.
 */
/*
 * gchar* g_strdelimit (gchar *string, const gchar *delimiters, gchar new_delimiter);
 * gchar** g_strsplit (const gchar *string, const gchar *delimiter, gint max_tokens);
 */
	PurpleConversationUiOps *ops;
	GSList *l;
	char *room = NULL;
	GList *plonks = NULL;
	GList *members = NULL;
	gchar **tmp;
	if(!args[0]) {
		purple_debug_info("Plonkers", "Bad arg: %s\n", args[0]);
		return PURPLE_CMD_RET_FAILED;
	}
	if(!g_utf8_validate(*args, -1, NULL)) {
		purple_debug_info("Plonkers", "Invalid UTF8: %s\n", args[0]);
		return PURPLE_CMD_RET_FAILED;
	}
	purple_debug_info("plonkers", "Plonk arg: %s\n", args[0]);
	g_strdelimit (*args, "_-|> <.,:;", ' ');
	purple_debug_info("plonkers", "Plonk delimited arg: %s\n", args[0]);
	tmp = g_strsplit(args[0], " ", 0);
	purple_debug_info("plonkers", "Plonk strsplit length: %i\n", g_strv_length(tmp));
	/* next step, remove duplicates in the array */

	ops = purple_conversation_get_ui_ops(c);
	
	PurpleAccount *account = purple_conversation_get_account(c);
	members = purple_conv_chat_get_users(PURPLE_CONV_CHAT(c));
	for (l = account->deny; l != NULL; l = l->next) {
		for (plonks = members; plonks; plonks = plonks->next) {
			if (!purple_utf8_strcasecmp((char *)l->data, plonks->data)) {
				purple_debug_info("plonkers", "Ignoring room member %s in room %s\n" ,plonks->data, room);
/*				purple_conv_chat_ignore(PURPLE_CONV_CHAT(c),plonks->data);
 *				ops->chat_update_user((c), plonks->data); */
			}
		}
	}
	g_list_free(plonks);
	g_list_free(members);
	g_strfreev(tmp);
	return PURPLE_CMD_RET_OK;
}

/*******************************************************************************
 * Prefs stuff
 ******************************************************************************/
static GtkWidget *
plonkers_make_label(const gchar *text, GtkSizeGroup *sg) {
	GtkWidget *label;

	label = gtk_label_new_with_mnemonic(text);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	if(sg)
		gtk_size_group_add_widget(sg, label);

	return label;
}

static GtkWidget *
plonkers_get_config_frame(PurplePlugin *plugin) {
	GtkWidget *vbox, *hbox, *frame, *label;
	GtkSizeGroup *sg;

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);

	frame = pidgin_make_frame(vbox, "Ignored Plonkers");

	pidgin_prefs_labeled_entry(frame, "Plonkers singular format:",
								 "/plugins/core/plugin_pack/plonkers/plonkers/format_singular",
								 NULL);
	pidgin_prefs_labeled_entry(frame, "Plonkers plural format:",
								 "/plugins/core/plugin_pack/plonkers/plonkers/format_plural",
								 NULL);

	frame = pidgin_make_frame(vbox, "Plonking");
	pidgin_prefs_labeled_entry(frame, "Plonked singular plural:",
								 "/plugins/core/plugin_pack/plonkers/plonked/format_singular",
								 NULL);
	pidgin_prefs_labeled_entry(frame, "Plonked plural format:",
								 "/plugins/core/plugin_pack/plonkers/plonked/format_plural",
								 NULL);
	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);


	frame = pidgin_make_frame(vbox, "Format information");
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(frame), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	label = plonkers_make_label("%P: List of plonkers", sg);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	label = plonkers_make_label("%N: Number of plonkers", NULL);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);


	gtk_widget_show_all(vbox);

	return vbox;
}

/*******************************************************************************
 * Stock stuff
 ******************************************************************************/

/*******************************************************************************
 * Plugin stuff
 ******************************************************************************/
static gboolean
plonkers_load(PurplePlugin *plugin) {
	const gchar *help = "<pre>plonkers;\n"
						"Tell people in a chat what you really think of them\n</pre>";

	/* register our command */
	plonkers_cmd = purple_cmd_register("plonkers", "", PURPLE_CMD_P_PLUGIN,
								PURPLE_CMD_FLAG_CHAT, NULL,
								plonkers_cmd_cb, help, NULL);
	plonk_cmd = purple_cmd_register("plonk", "s", PURPLE_CMD_P_PLUGIN,
								PURPLE_CMD_FLAG_CHAT|PURPLE_CMD_FLAG_IM, NULL,
								plonk_cmd_cb, help, NULL);

	return TRUE;
}

static gboolean
plonkers_unload(PurplePlugin *plugin) {
	/* remove our command */
	purple_cmd_unregister(plonkers_cmd);
	purple_cmd_unregister(plonk_cmd);

	return TRUE;
}

static void
init_plugin(PurplePlugin *plugin) {
	purple_prefs_add_none("/plugins/core/plugin_pack");
	purple_prefs_add_none("/plugins/core/plugin_pack/plonkers");
	purple_prefs_add_none("/plugins/core/plugin_pack/plonkers/plonkers");
	purple_prefs_add_string("/plugins/core/plugin_pack/plonkers/plonkers/format_singular",
						  "/me has identified %N plonker: %P.");
	purple_prefs_add_string("/plugins/core/plugin_pack/plonkers/plonkers/format_plural",
						  "/me has identified %N plonkers: %P.");
	purple_prefs_add_none("/plugins/core/plugin_pack/plonkers/plonked");
	purple_prefs_add_string("/plugins/core/plugin_pack/plonkers/plonked/format_singular",
						  "/me plonks: %P.");
	purple_prefs_add_string("/plugins/core/plugin_pack/plonkers/plonked/format_plural",
						  "/me plonks: %P.");

}

static PidginPluginUiInfo ui_info = {
	plonkers_get_config_frame,
	0,
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginInfo plonkers_info = {
	PURPLE_PLUGIN_MAGIC,								/* Fear			*/
	PURPLE_MAJOR_VERSION,								/* the			*/
	PURPLE_MINOR_VERSION,								/* reaper		*/
	PURPLE_PLUGIN_STANDARD,								/* type			*/
	PIDGIN_PLUGIN_TYPE,									/* ui requirement	*/
	0,													/* flags			*/
	NULL,												/* dependencies	*/
	PURPLE_PRIORITY_DEFAULT,							/* priority		*/

	"core-plugin_pack-Plonkers",						/* id			*/
	"Plonkers",											/* name			*/
	PP_VERSION,											/* version		*/
	"Tell plonkers what you really think",				/* summary		*/
	"A small plugin that lets you announce"
	" to a chat room your current ignores, as"
	" well as providing other pointless ingore and"
	" privacy tools for dealing with idiots.\n"
	"Name inspired by en_IE/GB word for 'idiots'.",		/* description	*/
	"Peter Lawler <bleeter from users.sf.net>",			/* author		*/
	PP_WEBSITE,											/* homepage		*/
	plonkers_load,										/* load			*/
	plonkers_unload,									/* unload		*/
	NULL,												/* destroy		*/

	&ui_info,											/* ui info		*/
	NULL,												/* extra info		*/
	NULL,												/* prefs info		*/
	NULL,												/* actions info	*/
	NULL,												/* reserved 1	*/
	NULL,												/* reserved 2	*/
	NULL,												/* reserved 3	*/
	NULL												/* reserved 4	*/
};

PURPLE_INIT_PLUGIN(plonkers, init_plugin, plonkers_info)

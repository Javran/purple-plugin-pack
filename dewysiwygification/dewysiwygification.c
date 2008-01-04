/*
 * DeWYSIWYGification - Lets you type in HTML without it being escaped to entities.
 * Copyright (C) 2004-2008 Tim Ringenbach <omarvo@hotmail.com>
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

/* If you can't figure out what this line is for, DON'T TOUCH IT. */
#include "../common/pp_internal.h"

#include <debug.h>
#include <plugin.h>
#include <signals.h>
#include <util.h>
#include <version.h>

#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <strings.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#define DEWYSIWYGIFICATION_PLUGIN_ID "dewysiwygification"


static gboolean
substitute_words_send_im(PurpleAccount *account, const char *receiver,
                       char **message)
{
	char *tmp;

	if (message == NULL || *message == NULL)
		return FALSE;

	tmp = purple_unescape_html(*message);
	g_free(*message);
	*message = tmp;

	purple_debug_misc("dewysiwygification", "it's now: %s", tmp);

	return FALSE;
}

static gboolean
substitute_words_send_chat(PurpleAccount *account, char **message, int id)
{
	char *tmp;

	if (message == NULL || *message == NULL)
		return FALSE;

	tmp = purple_unescape_html(*message);
	g_free(*message);
	*message = tmp;

	purple_debug_misc("dewysiwygification", "it's now: %s", tmp);

	return FALSE;
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	void *conv_handle = purple_conversations_get_handle();


	purple_signal_connect(conv_handle, "sending-im-msg",
						plugin, PURPLE_CALLBACK(substitute_words_send_im), NULL);
	purple_signal_connect(conv_handle, "sending-chat-msg",
						plugin, PURPLE_CALLBACK(substitute_words_send_chat), NULL);

	return TRUE;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,
	DEWYSIWYGIFICATION_PLUGIN_ID,
	N_("DeWYSIWYGification Plugin"),
	PP_VERSION,
	N_("Lets you type in HTML without it being escaped to entities."),
	N_("Lets you type in HTML without it being escaped to entities. This will not work well for some protocols. Use \"&lt;\" for a literal \"<\"."),
	"Tim Ringenbach <omarvo@hotmail.com>",
	PP_WEBSITE,
	plugin_load,
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

	info.name = _(info.name);
	info.summary = _(info.summary);
	info.description  = _(info.description);
}

PURPLE_INIT_PLUGIN(dewysiwygification, init_plugin, info)

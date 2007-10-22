/*
 * Buddy Time - Displays a buddy's local time
 *
 * A libpurple plugin that allows you to configure a timezone on a per-contact
 * basis so it can display the localtime of your contact when a conversation
 * starts. Convenient if you deal with contacts from many parts of the
 * world.
 *
 * Copyright (C) 2006-2007, Richard Laager <rlaager@pidgin.im>
 * Copyright (C) 2006, Martijn van Oosterhout <kleptog@svana.org>
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
#include "../common/pp_internal.h"

#define PLUGIN_STATIC_NAME   "gtkbuddytime"
#define PLUGIN_ID            PIDGIN_UI "-buddytime"

#include "buddytime.h"

#include <glib.h>

#include "plugin.h"
#include "version.h"

#include "gtkblist.h"
#include "gtkplugin.h"

PurplePlugin *core_plugin = NULL;

static void
buddytimezone_tooltip_cb(PurpleBlistNode * node, char **text, gboolean full, void *data)
{
    char *newtext;
    const char *timezone;
    struct tm tm;
    double diff;
    int ret;

    if (!full)
        return;

    timezone = purple_plugin_ipc_call(core_plugin, BUDDYTIME_BUDDY_GET_TIMEZONE,
                                      NULL, node, TRUE);

    if (!timezone)
        return;

    ret = GPOINTER_TO_INT(purple_plugin_ipc_call(core_plugin, BUDDYTIME_TIMEZONE_GET_TIME,
                                                 NULL, timezone, &tm, &diff));
    if (ret < 0)
        newtext = g_strdup_printf("%s\n<b>Timezone:</b> %s (error)", *text, timezone);
    else if (ret == 0)
    {
        const char *timetext = purple_time_format(&tm);

	if (diff < 0)
	{
            diff = 0 - diff;
            newtext = g_strdup_printf(dngettext(GETTEXT_PACKAGE,
                                               "%s\n<b>Local Time:</b> %s (%.4g hour behind)",
                                               "%s\n<b>Local Time:</b> %s (%.4g hours behind)", diff),
                                      *text, timetext, diff);
	}
	else
	{
            newtext = g_strdup_printf(dngettext(GETTEXT_PACKAGE,
                                               "%s\n<b>Local Time:</b> %s (%.4g hour ahead)",
                                               "%s\n<b>Local Time:</b> %s (%.4g hours ahead)", diff),
                                      *text, timetext, diff);	}
    }
    else
        return;

    g_free(*text);
    *text = newtext;
}

static gboolean
plugin_load(PurplePlugin * plugin)
{
    purple_signal_connect(pidgin_blist_get_handle(), "drawing-tooltip", plugin,
                        PURPLE_CALLBACK(buddytimezone_tooltip_cb), NULL);

    core_plugin = purple_plugins_find_with_id(CORE_PLUGIN_ID);

    return (core_plugin != NULL);
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	0,
	PURPLE_PLUGIN_STANDARD,          /**< type           */
	PIDGIN_PLUGIN_TYPE,              /**< ui_requirement */
	PURPLE_PLUGIN_FLAG_INVISIBLE,    /**< flags          */
	NULL,                            /**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,         /**< priority       */
	PLUGIN_ID,                       /**< id             */
	NULL,                            /**< name           */
	PP_VERSION,                      /**< version        */
	NULL,                            /**< summary        */
	NULL,                            /**< description    */
	PLUGIN_AUTHOR,	                 /**< author         */
	PP_WEBSITE,                      /**< homepage       */
	plugin_load,                     /**< load           */
	NULL,                            /**< unload         */
	NULL,                            /**< destroy        */
	NULL,                            /**< ui_info        */
	NULL,                            /**< extra_info     */
	NULL,                            /**< prefs_info     */
	NULL,                            /**< actions        */
	NULL,                            /**< reserved 1     */
	NULL,                            /**< reserved 2     */
	NULL,                            /**< reserved 3     */
	NULL                             /**< reserved 4     */
};

static void
init_plugin(PurplePlugin * plugin)
{
	info.dependencies = g_list_append(info.dependencies, CORE_PLUGIN_ID);

#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Buddy Time (Pidgin UI)");
	info.summary = _("Pidgin user interface for the Buddy Time plugin.");
	info.description = _("Pidgin user interface for the Buddy Time plugin.");
}

PURPLE_INIT_PLUGIN(PLUGIN_STATIC_NAME, init_plugin, info);

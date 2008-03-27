/* Gaim Colorize Plug-in v0.2
 *
 * Colorizes outgoing text to a gradient of specified starting and
 * and ending values.
 *
 * TODO:
 *    - echo color formatting to local color log
 *    - fix HTML-mixed messages (currently strips all HTML)
 *
 * Copyright (C) 2005, Ike Gingerich <ike_@users.sourceforge.net>
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

#include <gtk/gtk.h>
#include <config.h>

#ifndef GAIM_PLUGINS
#define GAIM_PLUGINS
#endif

#include <internal.h>
#include <version.h>
#include <util.h>
#include <debug.h>

static const guchar default_initial_rgb[3] = { 0xFF, 0x00, 0x00 };
static const guchar default_target_rgb[3]  = { 0x00, 0x00, 0x00 };

/* set up preferences dialog */
static GaimPluginPrefFrame* init_pref_frame(GaimPlugin *plugin)
{
   GaimPluginPrefFrame *frame;
   GaimPluginPref *ppref;

   frame = gaim_plugin_pref_frame_new();

   ppref = gaim_plugin_pref_new_with_label("Initial Color");
   gaim_plugin_pref_frame_add(frame, ppref);

   ppref = gaim_plugin_pref_new_with_name_and_label("/plugins/core/colorize/initial_r", "Red intensity (0-255): ");
   gaim_plugin_pref_set_bounds(ppref, 0, 255);
   gaim_plugin_pref_frame_add(frame, ppref);

   ppref = gaim_plugin_pref_new_with_name_and_label("/plugins/core/colorize/initial_g", "Green intensity (0-255): ");
   gaim_plugin_pref_set_bounds(ppref, 0, 255);
   gaim_plugin_pref_frame_add(frame, ppref);

   ppref = gaim_plugin_pref_new_with_name_and_label("/plugins/core/colorize/initial_b", "Blue intensity (0-255): ");
   gaim_plugin_pref_set_bounds(ppref, 0, 255);
   gaim_plugin_pref_frame_add(frame, ppref);

   ppref = gaim_plugin_pref_new_with_label("Target Color");
   gaim_plugin_pref_frame_add(frame, ppref);

   ppref = gaim_plugin_pref_new_with_name_and_label("/plugins/core/colorize/target_r", "Red intensity (0-255): ");
   gaim_plugin_pref_set_bounds(ppref, 0, 255);
   gaim_plugin_pref_frame_add(frame, ppref);

   ppref = gaim_plugin_pref_new_with_name_and_label("/plugins/core/colorize/target_g", "Green intensity (0-255): ");
   gaim_plugin_pref_set_bounds(ppref, 0, 255);
   gaim_plugin_pref_frame_add(frame, ppref);

   ppref = gaim_plugin_pref_new_with_name_and_label("/plugins/core/colorize/target_b", "Blue intensity (0-255): ");
   gaim_plugin_pref_set_bounds(ppref, 0, 255);
   gaim_plugin_pref_frame_add(frame, ppref);

   return frame;
}

inline static int round_gfloat_to_guchar(gfloat f)
{
	return ((guchar)(f + 0.5f));
}

inline static int rgb_equals(guchar a[3], gfloat b[3])
{
	return ( a[0] == round_gfloat_to_guchar(b[0]) &&
	         a[1] == round_gfloat_to_guchar(b[1]) &&
	         a[2] == round_gfloat_to_guchar(b[2])  );
}

static void colorize_message(char **message)
{
	guint    i, len;
	gfloat   d_grad[3], grad[3];
	guchar   initial_rgb[3], target_rgb[3], last_rgb[3];
	gchar    *formatted_char, *tmp, *new_msg;

	g_return_if_fail(message   != NULL);
	g_return_if_fail(*message  != NULL);
	g_return_if_fail(**message != '\0');

	new_msg = g_strdup("");
	len     = strlen( *message );

	/* get colors from preferences */
	initial_rgb[0] = (guint)gaim_prefs_get_int("/plugins/core/colorize/initial_r");
	initial_rgb[1] = (guint)gaim_prefs_get_int("/plugins/core/colorize/initial_g");
	initial_rgb[2] = (guint)gaim_prefs_get_int("/plugins/core/colorize/initial_b");

	target_rgb[0]  = (guint)gaim_prefs_get_int("/plugins/core/colorize/target_r");
	target_rgb[1]  = (guint)gaim_prefs_get_int("/plugins/core/colorize/target_g");
	target_rgb[2]  = (guint)gaim_prefs_get_int("/plugins/core/colorize/target_b");

	/* initialize current gradient value */
	grad[0] = (gfloat)initial_rgb[0];
	grad[1] = (gfloat)initial_rgb[1];
	grad[2] = (gfloat)initial_rgb[2];

	/* determine the delta gradient value */
	d_grad[0] = (gfloat)(target_rgb[0] - initial_rgb[0]) / (gfloat)len;
	d_grad[1] = (gfloat)(target_rgb[1] - initial_rgb[1]) / (gfloat)len;
	d_grad[2] = (gfloat)(target_rgb[2] - initial_rgb[2]) / (gfloat)len;

	/* open initial font tag and format first character */
	formatted_char = g_strdup_printf("<font color=\"#%02x%02x%02x\">%c",
	                                   round_gfloat_to_guchar(grad[0]),
	                                   round_gfloat_to_guchar(grad[1]),
	                                   round_gfloat_to_guchar(grad[2]),
	                                                       *(*message));

	/* create a new string with the newly formatted char and free the old one */
	tmp = g_strconcat(new_msg, formatted_char, NULL);
	g_free(formatted_char);
	g_free(new_msg);

	new_msg = tmp;

	/* format each character one by one:
	*    (if it is not a space) AND
	*    (if it is not the same color as the last character)
	*/
	for(i=1; i<len; i++)
	{
		/* store last color */
		last_rgb[0] = round_gfloat_to_guchar(grad[0]);
		last_rgb[1] = round_gfloat_to_guchar(grad[1]);
		last_rgb[2] = round_gfloat_to_guchar(grad[2]);

		/* increment the gradient */
		grad[0] += d_grad[0];
		grad[1] += d_grad[1];
		grad[2] += d_grad[2];

		/* format next character appropriately */
		if( g_ascii_isspace ( *(*message+i) ) ||
		    rgb_equals(last_rgb, grad)          )
			formatted_char = g_strdup_printf("%c", *(*message+i));
		else
			formatted_char = g_strdup_printf("</font><font color=\"#%02x%02x%02x\">%c",
			                                   round_gfloat_to_guchar(grad[0]),
			                                   round_gfloat_to_guchar(grad[1]),
			                                   round_gfloat_to_guchar(grad[2]),
			                                                     *(*message+i));


		/* create a new string with the newly formatted char and free the old one */
		tmp = g_strconcat(new_msg, formatted_char, NULL);
		g_free(formatted_char);
		g_free(new_msg);

		new_msg = tmp;
	}

	/* close final font tag */
	new_msg = g_strconcat(new_msg, "</font>", NULL);

	/* return result */
	g_free(*message);
	*message = new_msg;
}

/* respond to a sending-im signal by replacing outgoing text
 * with colorized version
 */
static void sending_im_msg(GaimAccount *account, char *receiver,
                            char **message)
{
	char *stripped_message;

	/* strip any existing HTML */
	stripped_message = gaim_markup_strip_html(*message);
	g_free(*message);

	/* colorize the message with HTML font tags */
	*message = stripped_message;
	colorize_message(message);

	/* todo: additional conversation manipulation is going to be required to
           display the colorized version of the message locally */
}

/* register sendin-im signal */
static gboolean plugin_load(GaimPlugin *plugin)
{
	gaim_debug(GAIM_DEBUG_INFO, "colorize", "colorize plugin loaded.\n");

	gaim_signal_connect(gaim_conversations_get_handle(), "sending-im-msg",
                            plugin, GAIM_CALLBACK(sending_im_msg), NULL);


	return TRUE;
}

static gboolean plugin_unload(GaimPlugin *plugin)
{
	return TRUE;
}

static GaimPluginUiInfo prefs_info = {
	init_pref_frame
};

static GaimPluginInfo info =
{
	GAIM_PLUGIN_MAGIC,
	GAIM_MAJOR_VERSION,
	GAIM_MINOR_VERSION,
	GAIM_PLUGIN_STANDARD,                             /**< type           */
	NULL,                                             /**< ui_requirement */
	0,                                                /**< flags          */
	NULL,                                             /**< dependencies   */
	GAIM_PRIORITY_DEFAULT,                            /**< priority       */

	NULL,                                             /**< id             */
	N_("Colorize Plugin"),                            /**< name           */
	N_("pre-release"),                                /**< version        */
	                                                  /**  summary        */
	N_("Colorizes outgoing message text."),
	                                                  /**  description    */
	N_("Colorizes outgoing message text to a gradient of specified starting and ending RGB values."),
	"Ike Gingerich <ike_@users.sourceforge.net>",     /**< author         */
	GAIM_WEBSITE,                                     /**< homepage       */

	plugin_load,                                      /**< load           */
	plugin_unload,                                    /**< unload         */
	NULL,                                             /**< destroy        */

	NULL,                                             /**< ui_info        */
	NULL,                                             /**< extra_info     */
	&prefs_info,
	NULL
};

/* initialize default preferences */
static void init_plugin(GaimPlugin *plugin)
{
   gaim_prefs_add_none("/plugins/core/colorize");

   gaim_prefs_add_int("/plugins/core/colorize/initial_r", default_initial_rgb[0]);
   gaim_prefs_add_int("/plugins/core/colorize/initial_g", default_initial_rgb[1]);
   gaim_prefs_add_int("/plugins/core/colorize/initial_b", default_initial_rgb[2]);

   gaim_prefs_add_int("/plugins/core/colorize/target_r", default_target_rgb[0]);
   gaim_prefs_add_int("/plugins/core/colorize/target_g", default_target_rgb[1]);
   gaim_prefs_add_int("/plugins/core/colorize/target_b", default_target_rgb[2]);
}

GAIM_INIT_PLUGIN(colorize, init_plugin, info)

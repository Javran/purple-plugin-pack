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

#include "../common/pp_internal.h"

#include <version.h>
#include <plugin.h>
#include <util.h>
#include <debug.h>

#define PLUGIN_ID		"core-plugin_pack-colorize"
#define PLUGIN_AUTHOR	"Ike Gingerich <ike_@users.sourceforge.net>"

#define PREFS_PREFIX	"/plugins/core/" PLUGIN_ID
#define PREFS_I_RED		PREFS_PREFIX "/initial_r"
#define PREFS_I_GREEN	PREFS_PREFIX "/initial_g"
#define PREFS_I_BLUE	PREFS_PREFIX "/initial_b"
#define PREFS_T_RED		PREFS_PREFIX "/target_r"
#define PREFS_T_GREEN	PREFS_PREFIX "/target_g"
#define PREFS_T_BLUE	PREFS_PREFIX "/target_b"

static const guint8 default_initial_rgb[3] = { 0xFF, 0x00, 0x00 };
static const guint8 default_target_rgb[3]  = { 0x00, 0x00, 0x00 };

/* set up preferences dialog */
static PurplePluginPrefFrame *
init_pref_frame(PurplePlugin *plugin) {
	PurplePluginPrefFrame *frame;
	PurplePluginPref *ppref;

	frame = purple_plugin_pref_frame_new();

	/* initial color */
	ppref = purple_plugin_pref_new_with_label("Initial Color");
	purple_plugin_pref_frame_add(frame, ppref);

	/* initial red intensity */
	ppref = purple_plugin_pref_new_with_name_and_label(PREFS_I_RED,
												"Red intensity (0-255): ");
	purple_plugin_pref_set_bounds(ppref, 0, 255);
	purple_plugin_pref_frame_add(frame, ppref);

	/* initial green intensity */
	ppref = purple_plugin_pref_new_with_name_and_label(PREFS_I_GREEN,
												"Green intensity (0-255): ");
	purple_plugin_pref_set_bounds(ppref, 0, 255);
	purple_plugin_pref_frame_add(frame, ppref);

	/* initial blue intensity */
	ppref = purple_plugin_pref_new_with_name_and_label(PREFS_I_BLUE,
												"Blue intensity (0-255): ");
	purple_plugin_pref_set_bounds(ppref, 0, 255);
	purple_plugin_pref_frame_add(frame, ppref);

	/* target color */
	ppref = purple_plugin_pref_new_with_label("Target Color");
	purple_plugin_pref_frame_add(frame, ppref);

	/* target red intensity */
	ppref = purple_plugin_pref_new_with_name_and_label(PREFS_T_RED,
												"Red intensity (0-255): ");
	purple_plugin_pref_set_bounds(ppref, 0, 255);
	purple_plugin_pref_frame_add(frame, ppref);

	/* target green intensity */
	ppref = purple_plugin_pref_new_with_name_and_label(PREFS_T_GREEN,
												"Green intensity (0-255): ");
	purple_plugin_pref_set_bounds(ppref, 0, 255);
	purple_plugin_pref_frame_add(frame, ppref);

	/* target blue intensity */
	ppref = purple_plugin_pref_new_with_name_and_label(PREFS_T_BLUE,
												"Blue intensity (0-255): ");
	purple_plugin_pref_set_bounds(ppref, 0, 255);
	purple_plugin_pref_frame_add(frame, ppref);

	return frame;
}

inline static guint8
round_gfloat_to_guint8(gfloat f) {
	return ((guchar)(f + 0.5f));
}

inline static gint32
rgb_equals(guint8 a[3], gfloat b[3]) {
	return ( a[0] == round_gfloat_to_guint8(b[0]) &&
	         a[1] == round_gfloat_to_guint8(b[1]) &&
	         a[2] == round_gfloat_to_guint8(b[2])  );
}

static void
colorize_message(char **message) {
	guint    i, len;
	gfloat   d_grad[3], grad[3];
	guint8   initial_rgb[3], target_rgb[3], last_rgb[3];
	gchar    *formatted_char, *tmp, *new_msg;

	g_return_if_fail(message   != NULL);
	g_return_if_fail(*message  != NULL);
	g_return_if_fail(**message != '\0');

	new_msg = g_strdup("");
	len     = strlen( *message );

	/* get colors from preferences */
	initial_rgb[0] = (guint8)purple_prefs_get_int(PREFS_I_RED);
	initial_rgb[1] = (guint8)purple_prefs_get_int(PREFS_I_GREEN);
	initial_rgb[2] = (guint8)purple_prefs_get_int(PREFS_I_BLUE);

	target_rgb[0]  = (guint8)purple_prefs_get_int(PREFS_T_RED);
	target_rgb[1]  = (guint8)purple_prefs_get_int(PREFS_T_GREEN);
	target_rgb[2]  = (guint8)purple_prefs_get_int(PREFS_T_BLUE);

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
	                                   round_gfloat_to_guint8(grad[0]),
	                                   round_gfloat_to_guint8(grad[1]),
	                                   round_gfloat_to_guint8(grad[2]),
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
		last_rgb[0] = round_gfloat_to_guint8(grad[0]);
		last_rgb[1] = round_gfloat_to_guint8(grad[1]);
		last_rgb[2] = round_gfloat_to_guint8(grad[2]);

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
			                                   round_gfloat_to_guint8(grad[0]),
			                                   round_gfloat_to_guint8(grad[1]),
			                                   round_gfloat_to_guint8(grad[2]),
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
static void
sending_im_msg(PurpleAccount *account, gchar *receiver, gchar **message) {
	gchar *stripped_message;

	/* strip any existing HTML */
	stripped_message = purple_markup_strip_html(*message);
	g_free(*message);

	/* colorize the message with HTML font tags */
	*message = stripped_message;
	colorize_message(message);

	/* todo: additional conversation manipulation is going to be required to
           display the colorized version of the message locally */
}

/* register sendin-im signal */
static gboolean
plugin_load(PurplePlugin *plugin) {
	purple_signal_connect(purple_conversations_get_handle(), "sending-im-msg",
						  plugin, PURPLE_CALLBACK(sending_im_msg), NULL);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin) {
	return TRUE;
}

static PurplePluginUiInfo prefs_info = {
	init_pref_frame
};

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

	PLUGIN_ID,
	NULL,
	PP_VERSION,
	NULL,
	NULL,
	PLUGIN_AUTHOR,
	PP_WEBSITE,

	plugin_load,
	plugin_unload,
	NULL,

	NULL,
	NULL,
	&prefs_info,
	NULL
};

/* initialize default preferences */
static void
init_plugin(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	info.name = _("Colorize");
	info.summary = _("Colorizes outgoing message text.");
	info.description = _("Colorizes outgoing message text to a gradient of "
						 "specified starting and ending RGB values.");

	/* prefs */
	purple_prefs_add_none(PREFS_PREFIX);

	purple_prefs_add_int(PREFS_I_RED, default_initial_rgb[0]);
	purple_prefs_add_int(PREFS_I_GREEN, default_initial_rgb[1]);
	purple_prefs_add_int(PREFS_I_BLUE, default_initial_rgb[2]);

	purple_prefs_add_int(PREFS_T_RED, default_target_rgb[0]);
	purple_prefs_add_int(PREFS_T_GREEN, default_target_rgb[1]);
	purple_prefs_add_int(PREFS_T_BLUE, default_target_rgb[2]);
}

PURPLE_INIT_PLUGIN(colorize, init_plugin, info)

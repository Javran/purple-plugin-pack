/*
 * Caps-notification
 * Copyright (C) 2010  Eion Robb
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "../common/pp_internal.h"

#include <notify.h>
#include <plugin.h>
#include <debug.h>
#include <cmds.h>
#include <pluginpref.h>
#include <prefs.h>

#ifndef _PIDGIN_CONVERSATION_H_
typedef enum
{
	PIDGIN_UNSEEN_NONE,
	PIDGIN_UNSEEN_EVENT,
	PIDGIN_UNSEEN_NO_LOG,
	PIDGIN_UNSEEN_TEXT,
	PIDGIN_UNSEEN_NICK
} PidginUnseenState;
#endif

#ifdef WIN32
#	include <windows.h>
#else
#	include <sys/ioctl.h>
//#	include <linux/kd.h>
#	define KDGETLED 0x4B31  /* get current LED status */
#	define KDSETLED 0x4B32  /* set new LED status */
#	define LED_SCR  0x01    /* scroll lock */
#	define LED_NUM  0x02    /* num lock */
#	define LED_CAP  0x04    /* caps lock */
#endif

static guint flash_timeout = 0;
static gint flashes_remaining = 0;
static gboolean flash_state = FALSE;

static void
led_set(gboolean state)
{
	gboolean numlock;
	gboolean capslock;
	gboolean scrolllock;
#ifdef WIN32
	BYTE keyState[256];
	GetKeyboardState((LPBYTE) &keyState);
#endif
	
	numlock = purple_prefs_get_bool("/plugins/core/eionrobb-capsnot/numlock");
	capslock = purple_prefs_get_bool("/plugins/core/eionrobb-capsnot/capslock");
	scrolllock = purple_prefs_get_bool("/plugins/core/eionrobb-capsnot/scrolllock");
	
#ifdef WIN32
	if (numlock)
	{
		if ((state && !(keyState[VK_NUMLOCK] & 1)) ||
			(!state && (keyState[VK_NUMLOCK] & 1)))
		{
			keybd_event(VK_NUMLOCK, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
			keybd_event(VK_NUMLOCK, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
		}
	}
	if (capslock)
	{
		if ((state && !(keyState[VK_CAPITAL] & 1)) ||
			(!state && (keyState[VK_CAPITAL] & 1)))
		{
			keybd_event(VK_CAPITAL, 0x3a, KEYEVENTF_EXTENDEDKEY | 0, 0);
			keybd_event(VK_CAPITAL, 0x3a, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
		}
	}
	if (scrolllock)
	{
		if ((state && !(keyState[VK_SCROLL] & 1)) ||
			(!state && (keyState[VK_SCROLL] & 1)))
		{
			keybd_event(VK_SCROLL, 0x46, KEYEVENTF_EXTENDEDKEY | 0, 0);
			keybd_event(VK_SCROLL, 0x46, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
		}
	}
#else
	long int keyState;
	ioctl(0, KDGETLED, &keyState);
	if (numlock)
	{
		if (state)
			keyState = keyState | LED_NUM;
		else
			keyState = keyState & ~LED_NUM;
	}
	if (capslock)
	{
		if (state)
			keyState = keyState | LED_CAP;
		else
			keyState = keyState & ~LED_CAP;
	}
	if (scrolllock)
	{
		if (state)
			keyState = keyState | LED_SCR;
		else
			keyState = keyState & ~LED_SCR;
	}
	ioctl(0, KDSETLED, keyState);
#endif
}

static gboolean
flash_toggle(gpointer data)
{
	flash_state = !flash_state;
	flashes_remaining--;
	
	led_set(flash_state);
	
	if (flashes_remaining <= 0)
	{
		if (flash_state)
		{
			led_set(flash_state = FALSE);
		}
		return FALSE;
	}
	return TRUE;
}

static void
capsnot_conversation_updated(PurpleConversation *conv, 
								PurpleConvUpdateType type)
{
	gboolean has_unseen;
	gint flashcount;
	gint flashseconds;
	const char *im = NULL, *chat = NULL;
	
	if( type != PURPLE_CONV_UPDATE_UNSEEN ) {
		return;
	}
	
	im = purple_prefs_get_string("/plugins/core/eionrobb-capsnot/im");
	chat = purple_prefs_get_string("/plugins/core/eionrobb-capsnot/chat");
	
	if (im && purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM)
	{
		if (g_str_equal(im, "always"))
			has_unseen = (GPOINTER_TO_INT(purple_conversation_get_data(conv, "unseen-state")) >= PIDGIN_UNSEEN_TEXT);
		else if (g_str_equal(im, "hidden"))
			has_unseen = (GPOINTER_TO_INT(purple_conversation_get_data(conv, "unseen-count")) > 0);
		else
			return;
	} else if (chat && purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT)
	{
		if (g_str_equal(chat, "always"))
			has_unseen = (GPOINTER_TO_INT(purple_conversation_get_data(conv, "unseen-state")) >= PIDGIN_UNSEEN_TEXT);
		else if (g_str_equal(chat, "nick"))
			has_unseen = (GPOINTER_TO_INT(purple_conversation_get_data(conv, "unseen-state")) >= PIDGIN_UNSEEN_NICK);
		else
			return;
	} else
	{
		return;
	}
	
	flashcount = purple_prefs_get_int("/plugins/core/eionrobb-capsnot/flashcount");
	flashseconds = purple_prefs_get_int("/plugins/core/eionrobb-capsnot/flashseconds");
	
	if (has_unseen)
	{
		purple_timeout_remove(flash_timeout);
		flashes_remaining = flashcount * 2;
		flash_timeout = purple_timeout_add(flashseconds * 1000 / flashcount / 2, flash_toggle, NULL);
	} else {
		flashes_remaining = 0;
	}
}

static PurplePluginPrefFrame *
plugin_config_frame(PurplePlugin *plugin)
{
	PurplePluginPrefFrame *frame;
	PurplePluginPref *ppref;
	
	frame = purple_plugin_pref_frame_new();
	
	ppref = purple_plugin_pref_new_with_label(_("Inform about unread..."));
	purple_plugin_pref_frame_add(frame, ppref);
	
	ppref = purple_plugin_pref_new_with_name_and_label(
							"/plugins/core/eionrobb-capsnot/im",
							_("Instant Messages:"));
	purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
	purple_plugin_pref_add_choice(ppref, _("Never"), "never");
	purple_plugin_pref_add_choice(ppref, _("In hidden conversations"), "hidden");
	purple_plugin_pref_add_choice(ppref, _("Always"), "always");
	purple_plugin_pref_frame_add(frame, ppref);
	
	ppref = purple_plugin_pref_new_with_name_and_label(
							"/plugins/core/eionrobb-capsnot/chat",
							_("Chat Messages:"));
	purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
	purple_plugin_pref_add_choice(ppref, _("Never"), "never");
	purple_plugin_pref_add_choice(ppref, _("When my nick is said"), "nick");
	purple_plugin_pref_add_choice(ppref, _("Always"), "always");
	purple_plugin_pref_frame_add(frame, ppref);
	
	
	ppref = purple_plugin_pref_new_with_label(_("Keyboard LEDs:"));
	purple_plugin_pref_frame_add(frame, ppref);
	
	ppref = purple_plugin_pref_new_with_name_and_label(
							"/plugins/core/eionrobb-capsnot/numlock",
							_("Num Lock"));
	purple_plugin_pref_frame_add(frame, ppref);
	ppref = purple_plugin_pref_new_with_name_and_label(
							"/plugins/core/eionrobb-capsnot/capslock",
							_("Caps Lock"));
	purple_plugin_pref_frame_add(frame, ppref);
	ppref = purple_plugin_pref_new_with_name_and_label(
							"/plugins/core/eionrobb-capsnot/scrolllock",
							_("Scroll Lock"));
	purple_plugin_pref_frame_add(frame, ppref);
	
	
	ppref = purple_plugin_pref_new_with_label(_("Flash Rate:"));
	purple_plugin_pref_frame_add(frame, ppref);
	
	ppref = purple_plugin_pref_new_with_name_and_label(
							"/plugins/core/eionrobb-capsnot/flashcount",
							_("Number of flashes"));
	purple_plugin_pref_set_bounds(ppref, 1, 10);
	purple_plugin_pref_frame_add(frame, ppref);
	ppref = purple_plugin_pref_new_with_name_and_label(
							"/plugins/core/eionrobb-capsnot/flashseconds",
							_("Duration of flashes (seconds)"));
	purple_plugin_pref_set_bounds(ppref, 1, 10);
	purple_plugin_pref_frame_add(frame, ppref);
	
	return frame;
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	purple_signal_connect(purple_conversations_get_handle(),
	                      "conversation-updated", plugin,
	                      PURPLE_CALLBACK(capsnot_conversation_updated), NULL);
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	purple_signal_disconnect(purple_conversations_get_handle(),
	                         "conversation-updated", plugin,
	                         PURPLE_CALLBACK(capsnot_conversation_updated));
	
	purple_timeout_remove(flash_timeout);
	flashes_remaining = 0;
	led_set(flash_state = FALSE);
	
	return TRUE;
}

static PurplePluginUiInfo prefs_info = {
	plugin_config_frame,
	0,   /* page_num (Reserved) */
	NULL, /* frame (Reserved) */
	/* Padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginInfo info = {
    PURPLE_PLUGIN_MAGIC,
    2,
    0,
    PURPLE_PLUGIN_STANDARD,
    NULL,
    0,
    NULL,
    PURPLE_PRIORITY_DEFAULT,

    "eionrobb-capsnot",
    NULL,
    PP_VERSION,

    NULL,
    NULL,
    "Eion Robb <eionrobb@gmail.com>",
    PP_WEBSITE, /* URL */

    plugin_load,   /* load */
    plugin_unload, /* unload */
    NULL,          /* destroy */

    NULL,
    NULL,
    &prefs_info,
    NULL,
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
#endif
	
	info.name = _("Caps-notification");
	info.summary = _("Led notification on keyboards");
	info.description = _("Informs of new messages with the NumLock, CapsLock, or ScrollLock LEDs");


	purple_prefs_add_none("/plugins/core/eionrobb-capsnot");
	
	purple_prefs_add_string("/plugins/core/eionrobb-capsnot/im", "always");
	purple_prefs_add_string("/plugins/core/eionrobb-capsnot/chat", "nick");
	
	purple_prefs_add_bool("/plugins/core/eionrobb-capsnot/numlock", FALSE);
	purple_prefs_add_bool("/plugins/core/eionrobb-capsnot/capslock", FALSE);
	purple_prefs_add_bool("/plugins/core/eionrobb-capsnot/scrolllock", TRUE);
	
	purple_prefs_add_int("/plugins/core/eionrobb-capsnot/flashcount", 8);
	purple_prefs_add_int("/plugins/core/eionrobb-capsnot/flashseconds", 2);
}

PURPLE_INIT_PLUGIN(capsnot, init_plugin, info);

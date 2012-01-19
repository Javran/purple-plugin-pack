/*
 * irssi - Implements several irssi features for Purple
 * Copyright (C) 2005-2008 Gary Kramlich <grim@reaperworld.com>
 * Copyright (C) 2006-2008 John Bailey <rekkanoryo@rekkanoryo.org>
 * Copyright (C) 2006-2008 Sadrul Habib Chowdhury <sadrul@users.sourceforge.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02111-1301, USA.
 */

#include "../common/pp_internal.h"

#include <time.h>

#include <plugin.h>
#include <cmds.h>

#include "irssi.h"

/******************************************************************************
 * Globals
 *****************************************************************************/
static guint irssi_datechange_timeout_id = 0;
static gint lastday = 0;

/******************************************************************************
 * Helpers
 *****************************************************************************/
static gboolean
irssi_datechange_get_day_and_month(time_t *t, gint *day, gint *month) {
	time_t rt;
	struct tm *temp;

	rt = time(NULL);
	temp = localtime(&rt);

	if(!temp)
		return FALSE;

	if(t)
		*t = rt;

	if(day)
		*day = temp->tm_mday;

	if(month)
		*month = temp->tm_mon;

	return TRUE;
}

static gboolean
irssi_datechange_timeout_cb(gpointer data) {
	time_t t;
	GList *l = NULL;
	gint day = 0, month = 0;
	gchar buff[80];
	gchar *message = NULL, *new_year = NULL;

	if(!irssi_datechange_get_day_and_month(&t, &day, &month))
		return TRUE;

	if(day == lastday)
		return TRUE;

	lastday = day;

	l = purple_get_conversations();
	if(!l)
		return TRUE;

	if(day == 1 && month == 0 && purple_prefs_get_bool(SENDNEWYEAR_PREF))
		new_year = g_strdup(_("Happy New Year!"));

	strftime(buff, sizeof(buff), "%d %b %Y", localtime(&t));
	message = g_strdup_printf(_("Day changed to %s"), buff);

	for(; l; l = l->next) {
		PurpleConversation *conv = (PurpleConversation *)l->data;

		purple_conversation_write(conv, NULL, message,
				PURPLE_MESSAGE_NO_LOG | PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_ACTIVE_ONLY,
				t);

		if(new_year) {
			switch (purple_conversation_get_type(conv))
			{
				case PURPLE_CONV_TYPE_IM:
					purple_conv_im_send(PURPLE_CONV_IM(conv), new_year);
					break;
				case PURPLE_CONV_TYPE_CHAT:
					purple_conv_chat_send(PURPLE_CONV_CHAT(conv), new_year);
					break;
				default:
					break;
			}
		}
	}

	g_free(message);
	g_free(new_year);

	return TRUE;
}

/******************************************************************************
 * "API"
 *****************************************************************************/
void
irssi_datechange_init(PurplePlugin *plugin) {
	if(purple_prefs_get_bool(DATECHANGE_PREF)) {
		if(irssi_datechange_timeout_id != 0)
			purple_timeout_remove(irssi_datechange_timeout_id);
	
		if(!irssi_datechange_get_day_and_month(NULL, &lastday, NULL))
			lastday = 0;

		/* set this to get called every 30 seconds.
		 *
		 * Yes we only care about a day change, but i'd rather get it in the first
		 * 30 seconds of the change rather than nearly a min later.
		 */
		irssi_datechange_timeout_id = g_timeout_add(30000,
													irssi_datechange_timeout_cb,
													NULL);
	}
}

void
irssi_datechange_uninit(PurplePlugin *plugin) {
	if(irssi_datechange_timeout_id != 0)
		purple_timeout_remove(irssi_datechange_timeout_id);
}


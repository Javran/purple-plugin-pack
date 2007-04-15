/*
 * irssi - Implements several irssi features for Purple
 * Copyright (C) 2005-2007 Gary Kramlich <grim@reaperworld.com>
 * Copyright (C) 2007 John Bailey <rekkanoryo@rekkanoryo.org>
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

/* pp_config.h provides necessary definitions that help us find/do stuff */
#ifdef HAVE_CONFIG_H
# include "../pp_config.h"
#endif

#include <time.h>

#include <plugin.h>
#include <cmds.h>

#include "../common/i18n.h"

/******************************************************************************
 * Globals
 *****************************************************************************/
static guint irssi_datechange_timeout_id = 0;
static gint lastday = 0;

/******************************************************************************
 * Helpers
 *****************************************************************************/
static gint
irssi_datechange_get_day(time_t *t) {
	struct tm *temp;

	temp = localtime(t);

	if(!temp)
		return 0;

	return temp->tm_mday;
}

static gint
irssi_datechange_get_month(time_t *t) {
	struct tm *temp;

	temp = localtime(t);

	if(!temp)
		return 0;

	return temp->tm_mon;
}

static gboolean
irssi_datechange_timeout_cb(gpointer data) {
	time_t t;
	GList *l;
	gint newday;
	gchar buff[80];
	gchar *message;

	t = time(NULL);
	newday = irssi_datechange_get_day(&t);

	if(newday == lastday)
		return TRUE;

	strftime(buff, sizeof(buff), "%d %b %Y", localtime(&t));
	message = g_strdup_printf(_("Day changed to %s"), buff);

	for(l = purple_get_conversations(); l; l = l->next) {
		PurpleConversation *conv = (PurpleConversation *)l->data;

		purple_conversation_write(conv, NULL, message,
				PURPLE_MESSAGE_NO_LOG | PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_ACTIVE_ONLY,
				t);
		if ((irssi_datechange_get_day(&t) == 1) &&
				(irssi_datechange_get_month(&t) == 0))
		{
			const gchar *new_year = _("Happy New Year");
			if(conv->type == PURPLE_CONV_TYPE_IM)
				purple_conv_im_send(PURPLE_CONV_IM(conv), new_year);
			else if(conv->type == PURPLE_CONV_TYPE_CHAT)
				purple_conv_chat_send(PURPLE_CONV_CHAT(conv), new_year);
		}
	}

	g_free(message);

	lastday = newday;

	return TRUE;
}

/******************************************************************************
 * "API"
 *****************************************************************************/
void
irssi_datechange_init(PurplePlugin *plugin) {
	time_t t;

	if(irssi_datechange_timeout_id != 0)
		purple_timeout_remove(irssi_datechange_timeout_id);
	
	t = time(NULL);
	lastday = irssi_datechange_get_day(&t);

	/* set this to get called every 30 seconds.
	 *
	 * Yes we only care about a day change, but i'd rather get it in the first
	 * 30 seconds of the change rather than nearly a min later.
	 */
	irssi_datechange_timeout_id = g_timeout_add(30000,
												irssi_datechange_timeout_cb,
												NULL);
}

void
irssi_datechange_uninit(PurplePlugin *plugin) {
	if(irssi_datechange_timeout_id != 0)
		purple_timeout_remove(irssi_datechange_timeout_id);
}


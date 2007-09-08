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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02111-1301, USA.
 */

#include "../common/pp_internal.h"

#ifdef HAVE_REGEX_H
# include <regex.h>
#endif

#include <plugin.h>
#include <cmds.h>

#include "textfmt.h"

/******************************************************************************
 * Globals
 *****************************************************************************/
#define EXPRESSION	"(^|[ 	])(%s)([^ ]+)(%s)($|[ 	])"

enum {
	GROUP_ALL = 0,
	GROUP_LEADING,
	GROUP_START_TOKEN,
	GROUP_TEXT,
	GROUP_END_TOKEN,
	GROUP_TRAILING,
	GROUP_TOTAL
};

/******************************************************************************
 * Helpers
 *****************************************************************************/

#ifdef HAVE_REGEX_H

#define FORMAT(account, message) { \
	if(!(account)) \
		return FALSE; \
	\
	if(!(account)->gc) \
		return FALSE; \
	\
	if(!(account)->gc->flags & PURPLE_CONNECTION_HTML) \
		return FALSE; \
	\
	if(!(message)) \
		return FALSE; \
	\
	*(message) = irssi_textfmt_regex(*(message)); \
	\
	return FALSE; \
}

#define APPEND_GROUP(group) { \
	t = iter + matches[(group)].rm_so; \
	offset = matches[(group)].rm_eo - matches[(group)].rm_so; \
	str = g_string_append_len(str, t, offset); \
}

static gchar *
irssi_textfmt_regex_helper(gchar *message, const gchar *token,
						   const gchar *tag)
{
	GString *str = NULL;
	gchar *ret = NULL, *expr = NULL, *iter = message, *t = NULL;
	regex_t regex;
	regmatch_t matches[GROUP_TOTAL];

	/* build our expression */
	expr = g_strdup_printf(EXPRESSION, token, token);

	/* compile the expression.
	 * We may want to move this so we only do it once for each type, but this
	 * is pretty cheap, resource-wise.
	 */
	if(regcomp(&regex, expr, REG_EXTENDED) != 0) {
		g_free(expr);
		return message;
	}

	/* ok, we compiled fine, lets clean up our expression */
	g_free(expr);

	/* now let's replce the matches.
	 * If we don't have any, drop out right away.
	 */
	if(regexec(&regex, iter, GROUP_TOTAL, matches, 0) != 0)
		return message;

	/* create our GString.  Heh heh */
	str = g_string_new("");

	/* now loop through the string looking for more matches */
	do {
		gint offset = 0;

		if(matches[0].rm_eo == -1)
			break;

		/* append everything up to the match */
		str = g_string_append_len(str, iter, matches[GROUP_ALL].rm_so);

		/* determine the leading white space */
		APPEND_GROUP(GROUP_LEADING);

		/* append the starting tag */ 
		g_string_append_printf(str, "<%s>", tag);

		/* append the starting token */
		APPEND_GROUP(GROUP_START_TOKEN);

		/* append the text */
		APPEND_GROUP(GROUP_TEXT);

		/* append the ending token */
		APPEND_GROUP(GROUP_END_TOKEN);

		/* append the ending tag */
		g_string_append_printf(str, "</%s>", tag);
	
		/* append the trailing whitespace */
		APPEND_GROUP(GROUP_TRAILING);

		/* move iter to the end of the match */
		iter += matches[GROUP_ALL].rm_eo;
	} while(regexec(&regex, iter, GROUP_TOTAL, matches, REG_NOTBOL) == 0);

	/* at this point, iter is either the remains of the text, of a single null
	 * terminator.  So throw it onto the GString.
	 */
	str = g_string_append(str, iter);

	/* kill the passed in message */
	g_free(message);

	/* take off our GString */
	ret = str->str;
	g_string_free(str, FALSE);

	return ret;
}

static gchar *
irssi_textfmt_regex(gchar *message) {
	message = irssi_textfmt_regex_helper(message, "\\*", "b");
	message = irssi_textfmt_regex_helper(message, "/", "i");
	message = irssi_textfmt_regex_helper(message, "-", "s");
	message = irssi_textfmt_regex_helper(message, "_", "u");

#if 0
	/* don't add these until imhtml support them again */
	message = irssi_textfmt_regex_helper(message, "\\^", "sup");
	message = irssi_textfmt_regex_helper(message, "~", "sub");
	message = irssi_textfmt_regex_helper(message, "!", "pre");
#endif

	return message;
}

#endif

/******************************************************************************
 * Callbacks
 *****************************************************************************/
#ifdef HAVE_REGEX_H

static gboolean
irssi_textfmt_writing_cb(PurpleAccount *account, const gchar *who,
						 gchar **message, PurpleConversation *conv,
						 PurpleMessageFlags flags)
{
	if(flags & PURPLE_MESSAGE_SYSTEM)
		return FALSE;

	FORMAT(account, message);
}

static gboolean
irssi_textfmt_sending_im_cb(PurpleAccount *account, const gchar *receiver,
							gchar **message)
{
	FORMAT(account, message);
}

static gboolean
irssi_textfmt_sending_chat_cb(PurpleAccount *account, gchar **message, gint id) {
	FORMAT(account, message);
}

#endif

/******************************************************************************
 * "API"
 *****************************************************************************/
void
irssi_textfmt_init(PurplePlugin *plugin) {
#ifdef HAVE_REGEX_H
	void *handle;

	handle = purple_conversations_get_handle();

	purple_signal_connect(handle, "writing-im-msg", plugin,
						PURPLE_CALLBACK(irssi_textfmt_writing_cb), NULL);
	purple_signal_connect(handle, "writing-chat-msg", plugin,
						PURPLE_CALLBACK(irssi_textfmt_writing_cb), NULL);
	purple_signal_connect(handle, "sending-im-msg", plugin,
						PURPLE_CALLBACK(irssi_textfmt_sending_im_cb), NULL);
	purple_signal_connect(handle, "sending-chat-msg", plugin,
						PURPLE_CALLBACK(irssi_textfmt_sending_chat_cb), NULL);
#endif
}

void
irssi_textfmt_uninit(PurplePlugin *plugin) {
	/* Nothing to do here, purple kills our callbacks for us. */
}


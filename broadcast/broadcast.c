/*
 * Broadcast - Send an IM to your entire buddy list
 * Copyright (C) 2007 John Bailey <rekkanoryo@rekkanoryo.org>
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

#ifdef HAVE_CONFIG_H
# include "../pp_config.h"
#endif /* HAVE_CONFIG_H */

#define PURPLE_PLUGINS

#include <blist.h>
#include <debug.h>
#include <request.h>
#include <util.h>
#include <version.h>

#include "../common/i18n.h"

#define BROADCAST_CATEGORY "plugin pack: broadcast"

/* These two functions directly ripped from groupmsg and renamed (shamelessly,
 * I should add). */
static void
broadcast_request_cancel_cb(GList *list, const char *text)
{
	g_list_free(list);
}

static void
broadcast_request_send_cb(GList *list, const char *message)
{
	char *stripped;
	GList *l;
	PurpleBuddy *b;
	PurpleConversation *conv = NULL;

	purple_markup_html_to_xhtml(message, NULL, &stripped);

	for (l = list; l; l = l->next) {
		b = l->data;
		conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, b->account, b->name);
		if(conv) {
			if (b->account->gc->flags & PURPLE_CONNECTION_HTML)
				purple_conv_im_send(PURPLE_CONV_IM(conv), message);
			else
				purple_conv_im_send(PURPLE_CONV_IM(conv), stripped);
		}
	}

	g_free(stripped);
	g_list_free(list);
}

static void
broadcast_action_cb(PurplePluginAction *action)
{
	GList *buddies = NULL, *ltmp = NULL;
	GString *stmp = NULL;
	const gchar *bname = NULL, *balias = NULL;
	PurpleBlistNode *root = NULL, *g = NULL, *c = NULL;
	PurpleBuddy *b = NULL;
	PurpleBuddyList *blist = NULL;

	blist = purple_get_blist();
	root = blist->root;

	for(g = root; g && PURPLE_BLIST_NODE_IS_GROUP(g); g = g->next) {
		for (c = g->child; c; c = c->next) {
			if(!PURPLE_BLIST_NODE_IS_CHAT(c)) {
				buddies = g_list_append(buddies,
							purple_contact_get_priority_buddy((PurpleContact *)c));

				purple_debug_info(BROADCAST_CATEGORY, "added a buddy to queue\n");
			}
		}
	}

	if(buddies == NULL) {
		purple_debug_error(BROADCAST_CATEGORY, "no buddies in queue!\n");

		return;
	}

	if(g_list_length(buddies) > 6) { /* arbitrary number that I like */
		stmp = g_string_new(_("Your message will be sent to and possibly annoy"
							" EVERYONE on your buddy list!"));

		purple_debug_info(BROADCAST_CATEGORY,
				"too many buddies on list; showing generic message!\n");
	} else {
		stmp = g_string_new("Your message will be sent to these buddies:\n");

		for(ltmp = buddies; ltmp; ltmp = ltmp->next) {
			b = (PurpleBuddy *)(ltmp->data);
			bname = purple_buddy_get_alias(b);
			balias = purple_buddy_get_alias(b);

			g_string_append_printf(stmp, "    %s (%s)\n", balias, bname);

			purple_debug_info(BROADCAST_CATEGORY, "added %s (%s) to dialog string\n",
					balias, bname);
		}
	}

	purple_debug_info(BROADCAST_CATEGORY, "requesting message input\n");

	purple_request_input(action, _("Broadcast Spim"),
			_("Enter the message to send"), stmp->str, "", TRUE, FALSE, "html",
			_("Send"), G_CALLBACK(broadcast_request_send_cb), _("Cancel"),
			G_CALLBACK(broadcast_request_cancel_cb), NULL, NULL, NULL, buddies);

	g_string_free(stmp, TRUE);

	return;
}

static GList *
broadcast_actions(PurplePlugin *plugin, gpointer context)
{
	purple_debug_info(BROADCAST_CATEGORY, "creating the action list\n");

	return g_list_append(NULL, purple_plugin_action_new(_("Broadcast a Message"),
				broadcast_action_cb));
}

static gboolean
broadcast_load(PurplePlugin *plugin)
{
	purple_debug_misc(BROADCAST_CATEGORY, "broadcast_load called\n");

	return TRUE;
}

static PurplePluginInfo broadcast_info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,
	"core-plugin_pack-broadcast",
	NULL,
	PP_VERSION,
	NULL,
	NULL,
	"John Bailey <rekkanoryo@rekkanoryo.org>",
	PP_WEBSITE,
	broadcast_load,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	broadcast_actions,
	NULL,
	NULL,
	NULL,
	NULL
};


static void
broadcast_init(PurplePlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	broadcast_info.name = _("Broadcaster");
	broadcast_info.summary = _("Send an IM to everyone on your buddy list.");
	broadcast_info.description = _("Adds the ability to send an IM to every "
			"Purple Contact on your Buddy List");
}

PURPLE_INIT_PLUGIN(broadcast, broadcast_init, broadcast_info)


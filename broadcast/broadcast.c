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
# include "../gpp_config.h"
#endif /* HAVE_CONFIG_H */

#define GAIM_PLUGINS

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
	GaimBuddy *b;
	GaimConversation *conv = NULL;

	gaim_markup_html_to_xhtml(message, NULL, &stripped);

	for (l = list; l; l = l->next) {
		b = l->data;
		conv = gaim_conversation_new(GAIM_CONV_TYPE_IM, b->account, b->name);
		if(conv) {
			if (b->account->gc->flags & GAIM_CONNECTION_HTML)
				gaim_conv_im_send(GAIM_CONV_IM(conv), message);
			else
				gaim_conv_im_send(GAIM_CONV_IM(conv), stripped);
		}
	}

	g_free(stripped);
	g_list_free(list);
}

static void
broadcast_action_cb(GaimPluginAction *action)
{
	GList *buddies = NULL, *ltmp = NULL;
	GString stmp = NULL;
	const gchar *bname = NULL, *balias = NULL;
	GaimBlistNode *root = NULL, *g = NULL, *c = NULL;
	GaimBuddy *b = NULL;
	GaimBuddyList *blist = NULL;

	blist = gaim_get_blist();
	root = blist->root;

	for(g = root; g && GAIM_BLIST_NODE_IS_GROUP(g); g = g->next) {
		for (c = g->child; c; c = c->next) {
			if(!GAIM_BLIST_NODE_IS_CHAT(c)) {
				buddies = g_list_append(buddies,
							gaim_contact_get_priority_buddy((GaimContact *)c));

				gaim_debug_info(BROADCAST_CATEGORY, "added a buddy to queue\n");
			}
		}
	}

	if(buddies == NULL) {
		gaim_debug_error(BROADCAST_CATEGORY, "no buddies in queue!\n");

		return;
	}

	if(g_list_length(buddies) > 6) { /* arbitrary number that I like */
		stmp = g_string_new(_("Your message will be sent to and possibly annoy"
							" EVERYONE on your buddy list!"));

		gaim_debug_info(BROADCAST_CATEGORY,
				"too many buddies on list; showing generic message!\n");
	} else {
		stmp = g_string_new("Your message will be sent to these buddies:\n");

		for(ltmp = buddies; ltmp; ltmp = ltmp->next) {
			b = (GaimBuddy *)(ltmp->data);
			bname = gaim_buddy_get_alias(b);
			balias = gaim_buddy_get_alias(b);

			stmp = g_string_append_printf(stmp, "    %s (%s)\n", balias, bname);

			gaim_debug_info(BROADCAST_CATEGORY, "added %s (%s) to dialog string\n",
					balias, bname);
		}
	}

	gaim_debug_info(BROADCAST_CATEGORY, "requesting message input\n");

	gaim_request_input(action, _("Broadcast Spim"),
			_("Enter the message to send"), stmp->str, "", TRUE, FALSE, "html",
			_("Send"), G_CALLBACK(broadcast_request_send_cb), _("Cancel"),
			G_CALLBACK(broadcast_request_cancel_cb), buddies);

	g_string_free(stmp, TRUE);

	return;
}

static GList *
broadcast_actions(GaimPlugin *plugin, gpointer context)
{
	gaim_debug_info(BROADCAST_CATEGORY, "creating the action list\n");

	return g_list_append(NULL, gaim_plugin_action_new(_("Broadcast a Message"),
				broadcast_action_cb));
}

static gboolean
broadcast_load(GaimPlugin *plugin)
{
	gaim_debug_misc(BROADCAST_CATEGORY, "broadcast_load called\n");

	return TRUE;
}

static GaimPluginInfo broadcast_info =
{
	GAIM_PLUGIN_MAGIC,
	GAIM_MAJOR_VERSION,
	GAIM_MINOR_VERSION,
	GAIM_PLUGIN_STANDARD,
	NULL,
	0,
	NULL,
	GAIM_PRIORITY_DEFAULT,
	"core-plugin_pack-broadcast",
	NULL,
	GPP_VERSION,
	NULL,
	NULL,
	"John Bailey <rekkanoryo@rekkanoryo.org>",
	GPP_WEBSITE,
	broadcast_load,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	broadcast_actions
};


static void
broadcast_init(GaimPlugin *plugin) {
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	broadcast_info.name = _("Broadcaster");
	broadcast_info.summary = _("Send an IM to everyone on your buddy list.");
	broadcast_info.description = _("Adds the ability to send an IM to every "
			"Gaim Contact on your Buddy List");
}

GAIM_INIT_PLUGIN(broadcast, broadcast_init, broadcast_info)


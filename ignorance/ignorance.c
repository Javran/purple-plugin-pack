/*
 * Ignorance - Make up for deficiencies in Purple's privacy
 *
 * Copyright (c) 200?-2006 Levi Bard
 * Copyright (c) 2005-2006 Peter Lawler
 * Copyright (c) 2005-2006 John Bailey
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

#define PURPLE_PLUGINS

/* ignorance headers */
#include "ignorance_level.h"
#include "ignorance_internal.h"
#include "ignorance.h"
#include "ignorance_denizen.h"
#include "ignorance_violation.h"
#include "ignorance_rule.h"

/* GTK Purple headers */
#include <gtkplugin.h>
#include <gtkutils.h>

#include "interface.h"

/* libgaim headers */
#include <blist.h>
#include <cmds.h>
#include <conversation.h>
#include <gaim.h>
#include <plugin.h>
#include <privacy.h>
#include <signals.h>
#include <sound.h>
#include <util.h>
#include <version.h>

/* libc headers */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/* for OSes that can at least *pretend* to be decent... */
#ifndef _WIN32
# include <sys/wait.h>
# include <sys/types.h>
# include <sys/select.h>
# include <fcntl.h>
# include <sys/stat.h>
#endif

/* globals */
static GPtrArray *levels;

static ignorance_level* ignorance_get_default_level() {
	gint i = 0;
	ignorance_level *il;

	for(i = 0; i < levels->len; ++i) {
		il = (ignorance_level*)g_ptr_array_index(levels, i);

		if(!strcmp(il->name->str, "Default"))
			return il;
	}

	return NULL;
}

static gboolean	ignorance_user_match(ignorance_level *il, const GString *username){
	return (ignorance_level_has_denizen(il,username));
}

static ignorance_level* ignorance_get_user_level(const GString *username){
	gint i = 0;
	ignorance_level	*il = NULL;

	for(i = 0; i < levels->len; ++i){
		il = (ignorance_level*)g_ptr_array_index(levels, i);
		if(ignorance_user_match(il, username))
			return il;
	}

	return ignorance_get_default_level();
}

static void purple_buddy_add(gpointer key, gpointer value, gpointer user_data) {
	PurpleBuddy *buddy = (PurpleBuddy*)value;
	ignorance_level	*level = (ignorance_level*)user_data;
	PurpleAccount *account = NULL;
	gchar *name = NULL;
	GString	*tmp;
	
	if(buddy && level) {
		name = (gchar*)buddy->name;
		account = buddy->account;
		tmp = g_string_new(purple_account_get_protocol_id(account));
		g_string_append(tmp, purple_normalize_nocase(account, name));
		
		if(ignorance_get_user_level(tmp) == ignorance_get_default_level()) {
			ignorance_level_add_denizen(level, tmp);

			if(strstr(level->name->str, "WL")) {
				purple_privacy_deny_remove(account, name, FALSE);
				purple_privacy_permit_add(account, name, FALSE);
			} else if(strstr(level->name->str,"BL")) {
				purple_privacy_permit_remove(account, name, FALSE);
				purple_privacy_deny_add(account, name, FALSE);
			}
		}

		g_string_free(tmp, TRUE);
	} else {
		purple_debug_error("ignorance", "Bad arguments to purple_buddy_add\n");
	}
}

static gint buf_get_line(gchar *ibuf, gchar **buf, gint *position, gint len) {
	gint pos = *position,
		spos = pos;

	if (pos == len)
		return 0;

	while (ibuf[pos++] != '\n') {
		if (pos == len)
			return 0;
	}

	pos--;
	ibuf[pos] = 0;
	*buf = &ibuf[spos];
	pos++;
	*position = pos;

	return 1;
}

static gboolean import_curphoo_list() {
	gchar *buf, *ibuf;
	gint pnt = 0,
		rule_flags = IGNORANCE_APPLY_CHAT | IGNORANCE_APPLY_IM | IGNORANCE_APPLY_ENTERLEAVE;
	gsize size = 0;
	FILE *curphoofile;
	ignorance_level *curphoolevel = NULL;
	ignorance_rule *curphoorule = NULL;
	GString *tmp = NULL;

	buf = g_build_filename(g_get_home_dir(), ".curphoo", "ignore", NULL);

	if(!(curphoofile = fopen(buf, "r"))) {
		purple_debug_error("ignorance", "Unable to open %s\n", buf);
		g_free(buf);

		return FALSE;
	} else
		fclose(curphoofile);

	g_file_get_contents(buf, &ibuf, &size, NULL);

	tmp = g_string_new("CurphooBL");
	curphoolevel = ignorance_get_level_name(tmp);

	if(!curphoolevel) {
		purple_debug_info("ignorance", "Creating new Curphoo blacklist\n");
#ifdef HAVE_REGEX_H
		curphoorule = ignorance_rule_newp(g_string_new("Everything"),
				IGNORANCE_RULE_REGEX,(gchar*)".*", IGNORANCE_FLAG_FILTER,
				rule_flags, TRUE, NULL, NULL, NULL);
#else
		curphoorule=ignorance_rule_newp(g_string_new("Everything"),
				IGNORANCE_RULE_SIMPLETEXT, (gchar*)"", IGNORANCE_FLAG_FILTER,
				rule_flags, TRUE, NULL, NULL, NULL);
#endif
		curphoolevel = ignorance_level_new();
		curphoolevel->name = g_string_new(tmp->str);
		ignorance_level_add_rule(curphoolevel, curphoorule);
		ignorance_add_level(curphoolevel);
	}

	if(!tmp)
		tmp = g_string_new("");

	g_free(buf);

	purple_debug_info("ignorance", "Preparing to read in curphoo blacklist users\n");

	for(; buf_get_line(ibuf, &buf, &pnt, size); ){
		g_string_assign(tmp, "prpl-yahoo");
		g_string_append(tmp, purple_normalize_nocase(NULL,buf));

		if(ignorance_get_user_level(tmp) == ignorance_get_default_level())
			ignorance_level_add_denizen(curphoolevel, tmp);
	}

	g_free(ibuf);

	pnt = 0;
	buf = g_build_filename(g_get_home_dir(), ".curphoo", "buddies", NULL);

	g_file_get_contents(buf, &ibuf, &size, NULL);

	g_string_assign(tmp, "CurphooWL");

	curphoolevel = ignorance_get_level_name(tmp);

	if(!curphoolevel) {
		purple_debug_info("ignorance", "Creating new Curphoo whitelist\n");

		curphoolevel = ignorance_level_new();
		curphoolevel->name = g_string_new(tmp->str);
		ignorance_add_level(curphoolevel);
	}

	g_free(buf);

	purple_debug_info("ignorance", "Preparing to read in curphoo whitelist users\n");

	for(; buf_get_line(ibuf, &buf, &pnt, size); ) {
		g_string_assign(tmp, "prpl-yahoo");
		g_string_append(tmp, purple_normalize_nocase(NULL, buf));

		if(ignorance_get_user_level(tmp) == ignorance_get_default_level())
			ignorance_level_add_denizen(curphoolevel, tmp);
	}

	g_free(ibuf);

	purple_debug_info("ignorance", "Done importing Curphoo users\n");

	return TRUE;
}

static gboolean import_purple_list() {
	PurpleBuddyList *bl;
	GString *wlname;
	ignorance_level *wl;
	gboolean rv = FALSE;

	bl = purple_get_blist();
	wlname = g_string_new("WL");
	wl = ignorance_get_level_name(wlname);

	if(bl && wl) {
		g_hash_table_foreach(bl->buddies, purple_buddy_add, wl);
		rv = TRUE;
	} else
		purple_debug_error("ignorance", "Unable to get Purple buddy list!\n");

	g_string_free(wlname, TRUE);

	return rv;
}

static gboolean import_zinc_list()
{
	FILE *zincfile;
	gchar *buf, *ibuf;
	gint pnt = 0,
		rule_flags = IGNORANCE_APPLY_CHAT | IGNORANCE_APPLY_IM | IGNORANCE_APPLY_ENTERLEAVE;
	gsize size = 0;
	ignorance_level *zinclevel = NULL;
	ignorance_rule *zincrule = NULL;
	GString *tmp = NULL;

	buf = g_build_filename(g_get_home_dir(), ".zinc", "ignore", NULL);

	if(!(zincfile = fopen(buf, "r"))) {
		purple_debug_error("ignorance", "Unable to open %s\n",buf);

		g_free(buf);

		return FALSE;
	} else
		fclose(zincfile);

	g_file_get_contents(buf, &ibuf, &size, NULL);

	tmp = g_string_new("ZincBL");

	zinclevel = ignorance_get_level_name(tmp);

	if(!zinclevel){
		purple_debug_info("ignorance", "Creating new Zinc blacklist\n");
#ifdef HAVE_REGEX_H
		zincrule = ignorance_rule_newp(g_string_new("Everything"),
				IGNORANCE_RULE_REGEX, (gchar*)".*", IGNORANCE_FLAG_FILTER,
				rule_flags, TRUE, NULL, NULL, NULL);
#else
		zincrule = ignorance_rule_newp(g_string_new("Everything"),
				IGNORANCE_RULE_SIMPLETEXT, (gchar*)"", IGNORANCE_FLAG_FILTER,
				rule_flags, TRUE, NULL, NULL, NULL);
#endif
		zinclevel = ignorance_level_new();
		zinclevel->name = g_string_new(tmp->str);
		ignorance_level_add_rule(zinclevel, zincrule);
		ignorance_add_level(zinclevel);
	}

	if(!tmp)
		tmp=g_string_new("");

	g_free(buf);

	purple_debug_info("ignorance", "Preparing to read in zinc blacklist users\n");

	for(; buf_get_line(ibuf, &buf, &pnt, size); ) {
		g_string_assign(tmp, "prpl-yahoo");
		g_string_append(tmp, purple_normalize_nocase(NULL, buf));
		if(ignorance_get_user_level(tmp) == ignorance_get_default_level())
			ignorance_level_add_denizen(zinclevel, tmp);
	}

	g_free(ibuf);

	pnt = 0;
	buf = g_build_filename(g_get_home_dir(), ".zinc", "whitelist", NULL);

	g_file_get_contents(buf, &ibuf, &size, NULL);

	g_string_assign(tmp, "ZincWL");

	zinclevel = ignorance_get_level_name(tmp);

	if(!zinclevel) {
		purple_debug_info("ignorance", "Creating new Zinc whitelist\n");
		zinclevel = ignorance_level_new();
		zinclevel->name = g_string_new(tmp->str);
		ignorance_add_level(zinclevel);
	}

	g_free(buf);

	purple_debug_info("ignorance", "Preparing to read in zinc whitelist users\n");

	for(; buf_get_line(ibuf, &buf, &pnt, size); ) {
		/* maybe this will be this way eventually, when i kill unnecessary
		 * GStrings - rekkanoryo */
		/* tmp = g_strdup_printf("prpl-yahoo %s", purple_normalize_nocase(NULL, buf)); */
		g_string_assign(tmp, "prpl-yahoo");
		g_string_append(tmp, purple_normalize_nocase(NULL, buf));
		if(ignorance_get_user_level(tmp) == ignorance_get_default_level())
			ignorance_level_add_denizen(zinclevel, tmp);
	}

	g_free(ibuf);

	purple_debug_info("ignorance", "Done importing Zinc users\n");

	return TRUE;
}

static gboolean ignorance_rm_user(PurpleConversation *conv, const gchar *username) {
	gchar *msgbuf = NULL, *cursor = NULL;
	gboolean retval = FALSE;
	GString *usergs;
	ignorance_level *level = NULL;
	int len=0;

	usergs = g_string_new(purple_normalize_nocase(NULL, username));

	level=ignorance_get_user_level(usergs);

	if(level) {
		retval = ignorance_level_remove_denizen(level, usergs);

		purple_debug_info("ignorance", "Done removing denizen from level\n");

		if(conv) {
			purple_debug_info("ignorance",
					"Creating status message for username %x and level %x\n",
					username, level);

			if(retval) {
				msgbuf = g_strdup_printf(_("Successfully removed %s from %s"),
						username, level->name->str);

				retval = TRUE;
			} else
				msgbuf = g_strdup_printf(_("Unable to remove %s from %s\n"),
						username, level->name->str);

			purple_debug_info("ignorance", "Writing status message\n");

			purple_conversation_write(conv, NULL, msgbuf, PURPLE_MESSAGE_NO_LOG,
					time(NULL));

			g_free(msgbuf);
		}
	}

	if(conv) { 
		purple_debug_info("ignorance",
				"Preparing to push through to gaim privacy\n");

		len = strlen(purple_account_get_protocol_id(
					purple_conversation_get_account(conv)));

		cursor = (gchar*)username + len;

		if(cursor && (strlen(username) > len)) {
			purple_debug_info("ignorance", "Removing from permit list\n");

			purple_privacy_permit_remove(purple_conversation_get_account(conv),
					cursor, FALSE);

			purple_debug_info("ignorance", "Removing from deny list\n");

			purple_privacy_deny_remove(purple_conversation_get_account(conv), cursor,
					FALSE);

			if(purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT) {
				purple_debug_info("ignorance", "Removing from chat ignore list\n");

				purple_conv_chat_unignore(PURPLE_CONV_CHAT(conv), cursor);
			}
		}
	}

	g_string_free(usergs, TRUE);

	purple_debug_info("ignorance", "Exiting\n");

	return retval;
}

static gboolean
ignorance_place_user_name(const GString *level_name, const GString *username) {
	ignorance_level	*current_level = ignorance_get_user_level(username),
			*newlevel = ignorance_get_level_name(level_name);

	if(newlevel) {
		if(newlevel != current_level) {
			ignorance_level_remove_denizen(current_level, username);
			ignorance_level_add_denizen(newlevel, username);

			return TRUE;
		}
	} else
		purple_debug_error("ignorance", "Invalid level %s\n", level_name->str);

	return FALSE;
}

static gboolean ignorance_bl_user(PurpleConversation *conv, const gchar *username,
								  const gchar *actual_levelname)
{
	gboolean retval = FALSE;
	gchar *msgbuf = NULL;
	GString *wlname, *usergs;
	PurpleAccount *account = NULL;

	wlname = g_string_new(actual_levelname);

	g_return_val_if_fail(conv != NULL, retval);

	account = purple_conversation_get_account(conv);
	usergs = g_string_new(purple_account_get_protocol_id(account));
	g_string_append(usergs, purple_normalize_nocase(NULL, username));

	if(ignorance_place_user_name(wlname, usergs)){
		retval = TRUE;
		purple_privacy_permit_remove(account, username, FALSE);
		purple_privacy_deny_add(account, username, FALSE);

		if(purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT)
			purple_conv_chat_ignore(PURPLE_CONV_CHAT(conv), username);

		msgbuf = g_strdup_printf(_("Assigned user %s to %s"), username,
				actual_levelname);
	} else
		msgbuf = g_strdup_printf(
				_("Unable to assign user %s to %s - may already be there"),
				username, actual_levelname);

	purple_conversation_write(conv, NULL, msgbuf, PURPLE_MESSAGE_NO_LOG, time(NULL));

	g_free(msgbuf);
	g_string_free(usergs, TRUE);
	g_string_free(wlname, TRUE);

	return retval;
}

#ifndef _WIN32
static int read_nonblock(int fd,unsigned long len,unsigned long timeout,GString *inp){
	int chrs=0, timedout=FALSE, rv=0;
	unsigned long timeout_usec=timeout*1000000, tlapsed=0;
	static const int SLEEP_INTERVAL=50000;

	gchar *ptr=g_malloc((len+1)*sizeof(gchar));

	while (chrs < len){
		if (tlapsed > timeout_usec){
			timedout = TRUE;
			break;
		}
		if ((rv = read (fd, ptr, (len - chrs))) < 0){
			rv = errno;
			usleep (SLEEP_INTERVAL);
			tlapsed += SLEEP_INTERVAL;
			continue;
		} else if (rv == 0)
			break;
		chrs += rv;
		*(ptr + rv)= 0;
		g_string_append(inp,ptr);
		*ptr=0;
	}
	g_free(ptr);
	if(timedout)
		return -1;
	else
		return chrs;
}
#endif

static gchar *
yahoo_strip_tattoo(gchar *origmessage) {
	gchar *cursor = NULL, *cursor2 = NULL, *beginstack = NULL,
		*endstack = NULL, *message = NULL;
	gint rvindex;

	message = g_ascii_strdown(origmessage, -1);
	cursor = strstr(message, "<font");

	if(message == cursor) {
		cursor2 = strstr(message, "tattoo");

		if(cursor2) {
			cursor = strstr(cursor2, ">");

			for( ; cursor; ) {
				endstack = strstr(cursor, "</");
				beginstack = strstr(cursor, "<");

				if(endstack == NULL || beginstack== NULL) {
					cursor = NULL;
					break;
				}

				cursor = strstr(endstack, ">");

				if(beginstack == endstack)
					break;
			}

			if(cursor) {
				rvindex = cursor + 1 - message;

				purple_debug_info("yahoo", "%s\nconverted to \n%s\n%s\n\n",
					origmessage, cursor + 1, origmessage + rvindex);

				g_free(message);

				return origmessage + rvindex;
			}
		}
	}

	g_free(message);

	return origmessage;
}

static int
handle_exec_command (const gchar *command, GString *result,unsigned long maxlen) {
#ifndef _WIN32
	GString *inp=g_string_new("");
	int p[2], pid, chrs;

	pipe(p);

	chrs = inp->len;
	if ((pid = fork ()) == -1) {
		g_string_assign (result, command);
		g_string_append (result, ": couldn't fork");
		return -1;
	} else if (pid){
		int rv;
		int flags = fcntl (p[0], F_GETFL, 0);

		close (p[1]);
		fcntl (p[0], F_SETFL, flags | O_NONBLOCK);
		
		rv=read_nonblock(p[0],maxlen-chrs,EXEC_TIMEOUT,inp);	

		if (kill (pid, 0) == 0)
				 kill (pid, SIGKILL);
		if (rv < 0)
			g_string_append (inp, "[process timed out]");
		else if(maxlen==(rv+chrs))
			g_string_append(inp,"...");
		else if(('\n'==(inp->str)[inp->len-1]))
			g_string_truncate(inp,inp->len-1);

		g_string_assign (result, inp->str);

		g_string_free(inp,TRUE);
		waitpid (pid, NULL, 0);
	} else {
		close (p[0]);
		if (p[1] != STDOUT_FILENO){
			dup2 (p[1], STDOUT_FILENO);
			close (p[1]);
		}
		if (STDERR_FILENO != STDOUT_FILENO)
			dup2 (STDOUT_FILENO, STDERR_FILENO);
		execlp ("sh", "sh", "-c", command, NULL);
	}
	return 0;
#else
	gint retval;
	gchar *message = NULL;

	g_string_assign (result, command);
	g_string_append (result, ": ");

	if (G_WIN32_HAVE_WIDECHAR_API ()) {
		wchar_t *wc_cmd = g_utf8_to_utf16(command,
				-1, NULL, NULL, NULL);

		retval = (gint)ShellExecuteW(NULL,NULL,wc_cmd,NULL,NULL,SW_SHOWNORMAL);
		g_free(wc_cmd);
	} else {
		char *l_cmd = g_locale_from_utf8(command,
				-1, NULL, NULL, NULL);

		retval = (gint)ShellExecuteA(NULL,NULL,l_cmd,NULL,NULL,SW_SHOWNORMAL);
		g_free(l_cmd);
	}

	if (retval<=32) {
		message = g_win32_error_message(retval);
		g_string_append(result,message);
	}

	purple_debug_info("Ignorance", "Execute command called for: "
					"%s\n%s%s%s", command, retval ? "" : "Error: ",
					retval ? "" : message, retval ? "" : "\n");
	g_free(message);
	return 0;
#endif
}

static gboolean
apply_rule(PurpleConversation *conv, PurpleAccount *account,
		const GString *username, const GString *text, gint flags)
{
	gint text_score = 0;
	gboolean rv = TRUE, newconv = FALSE;
	GList *violations = NULL, *cursor = NULL;
	GString *tmp;
	GString *prpluser = g_string_new(purple_account_get_protocol_id(account));
	ignorance_level *user_level;
	ignorance_violation *viol;
	PurpleConversationType conv_type;

	g_string_append(prpluser, purple_normalize_nocase(account, username->str));
	user_level=ignorance_get_user_level(prpluser);

	purple_debug_info("ignorance", "Preparing to check %s\n", text->str);

	text_score=ignorance_level_rulecheck(user_level, prpluser, text, flags,
			&violations);

	purple_debug_info("ignorance", "Got score %d\n", text_score);

	if(!(text_score & (IGNORANCE_FLAG_FILTER | IGNORANCE_FLAG_IGNORE))) {
		rv = FALSE;
		if(text_score) {
			for(cursor = violations; cursor; cursor = cursor->next) {
				viol = (ignorance_violation*)(cursor->data);
				purple_debug_info("ignorance", "Got violation type %d: %s\n",
						viol->type,viol->value);

				switch(viol->type) {
					case IGNORANCE_FLAG_SOUND:
						purple_debug_info("ignorance",
								"Attempting to play sound %s\n", viol->value);

						purple_sound_play_file(viol->value, account);

						break;

					case IGNORANCE_FLAG_EXECUTE:
						purple_debug_info("ignorance",
								"Attempting to execute command %s\n",
								viol->value);

						tmp = g_string_new("");

						handle_exec_command(viol->value, tmp, 512);

						if(conv)
							purple_conversation_write(conv,
									purple_account_get_username(account),
									tmp->str, PURPLE_MESSAGE_NO_LOG, time(NULL));

						g_string_free(tmp, TRUE);

						break;

					case IGNORANCE_FLAG_MESSAGE:
						if(!conv) {
							newconv = TRUE;
							conv = purple_conversation_new(PURPLE_CONV_TYPE_IM,
									account, username->str);
						}

						conv_type = purple_conversation_get_type(conv);

						if(conv_type == PURPLE_CONV_TYPE_IM) {
							purple_conv_im_send(PURPLE_CONV_IM(conv), viol->value);
						} else if(conv_type == PURPLE_CONV_TYPE_CHAT) {
							purple_conv_chat_send(PURPLE_CONV_CHAT(conv),
									viol->value);
						} /* braces for readability only */

						if(newconv)
							purple_conversation_destroy(conv);

						break;

					default:
						break;
				}
			}
		}
	} else if(text_score & IGNORANCE_FLAG_IGNORE)
		ignorance_bl_user(conv, username->str, "BL");

	g_string_free(prpluser, TRUE);

	purple_debug_info("ignorance", "Preparing to free violation items\n");
	g_list_foreach(violations, ignorance_violation_free_g, NULL);

	purple_debug_info("ignorance",
			"Done freeing violation items, now freeing list itself\n");
	g_list_free(violations);

	purple_debug_info("ignorance",
			"Done checking, returning from applying rules\n");

	return rv;
}

static gboolean substitute (PurpleConversation *conv,PurpleAccount *account,
							const gchar *sender, gchar **message, gint msgflags) {
	GString *username=NULL, *text=NULL;
	gboolean rv=FALSE;
	gchar *newmsg, *cursor;

	if (NULL == message)
		return FALSE;
	else if (NULL == (*message))
		return FALSE;


	username=g_string_new(purple_normalize_nocase(account,sender));

	purple_debug_info("ignorance","Got message \"%s\" from user \"%s\"\n",*message,sender);

	cursor=yahoo_strip_tattoo(*message);
	if(cursor!=*message){
		newmsg=g_strdup(cursor);
		g_free(*message);
		(*message)=newmsg;
	}

	text=g_string_new(*message);

	rv=apply_rule(conv,account,username,text,msgflags);

	if(rv)
		purple_debug_info("ignorance", "%s: %s violated!\n",username->str,text->str);

	g_string_free(username,TRUE);
	g_string_free(text,TRUE);

	purple_debug_info("ignorance","Returning from substitution\n");

	return rv;
}

static gboolean chat_cb(PurpleAccount *account, gchar **sender, gchar **buffer,
						PurpleConversation *chat, void *data){
	return substitute(chat, account, *sender, buffer,
					  IGNORANCE_APPLY_CHAT | IGNORANCE_APPLY_USER);
}

static gboolean im_cb(PurpleAccount *account, gchar **sender, gchar **buffer,
					  gint *flags, void *data){
	PurpleConversation *gc;

	gc=purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM,*sender,account);

	return substitute(gc, account, *sender,buffer,
					  IGNORANCE_APPLY_IM | IGNORANCE_APPLY_USER);
}

static gboolean chat_joinleave_cb (PurpleConversation *conv, const gchar *name,
								   void *data){
	gchar *message=g_strdup(name);
	gboolean rv=substitute(conv, purple_conversation_get_account(conv), name,
						   &message,IGNORANCE_APPLY_ENTERLEAVE | IGNORANCE_APPLY_USER);
	g_free(message);

	return rv;
}

static gint chat_invited_cb(PurpleAccount *account,const gchar *inviter, const gchar *chat, const gchar *invite_message, const GHashTable *components, void *data){
	gchar *message=g_strdup(invite_message);
	gboolean rv;
	gint invite_ask=0;
	PurpleConversation *gc;
	g_free(message);

	gc=purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM,inviter,account);

	rv=substitute(gc,account,inviter,&message,IGNORANCE_APPLY_INVITE | IGNORANCE_APPLY_USER | IGNORANCE_APPLY_CHAT);

	if(rv)
		invite_ask=-1;

	return invite_ask;
}

static void buddy_added_cb(PurpleBuddy *buddy, gpointer data){
	GString	*wlname=g_string_new("WL");
	ignorance_level	*wl=ignorance_get_level_name(wlname);
	PurpleAccount *account=buddy->account;
	gchar *name=(gchar*)buddy->name;

	purple_debug_info("ignorance","Caught buddy-added for %s%s\n",
		purple_account_get_protocol_id(account), name);
	purple_buddy_add(NULL,buddy,wl);

	g_string_free(wlname,TRUE);
}

static void buddy_removed_cb(PurpleBuddy *buddy, gpointer data){
	GString *tmp=NULL;
	PurpleConversation *conv=NULL;
	PurpleAccount *account=buddy->account;
	gchar *name=(gchar*)buddy->name;

	tmp=g_string_new(purple_account_get_protocol_id(account));

	conv=purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM,tmp->str,account);

	purple_debug_info("ignorance","Caught buddy-removed for %s%s\n",
		purple_account_get_protocol_id(account), name);

	g_string_append(tmp,purple_normalize_nocase(account,name));

	ignorance_rm_user(conv,tmp->str);

	g_string_free(tmp,TRUE);
}

static gboolean generate_default_levels() {
	ignorance_level *tmplvl;
	ignorance_rule *tmprule;
	GString	*tmpgs;

	tmplvl=ignorance_level_new();
	tmplvl->name=g_string_new("Default");
	ignorance_add_level(tmplvl);

	tmplvl=ignorance_level_new();
	tmplvl->name=g_string_new("WL");
	ignorance_add_level(tmplvl);

	tmplvl=ignorance_level_new();
	tmplvl->name=g_string_new("BL");
	ignorance_add_level(tmplvl);

	tmpgs=g_string_new("Everything");
#ifdef HAVE_REGEX_H
	tmprule=ignorance_rule_newp(tmpgs,IGNORANCE_RULE_REGEX,".*",
								IGNORANCE_FLAG_FILTER,
								IGNORANCE_APPLY_CHAT | IGNORANCE_APPLY_IM,
								TRUE,NULL,NULL,NULL);
#else
	tmprule=ignorance_rule_newp(tmpgs,IGNORANCE_RULE_SIMPLETEXT,"",
								IGNORANCE_FLAG_FILTER,
								IGNORANCE_APPLY_CHAT | IGNORANCE_APPLY_IM,
								TRUE,NULL,NULL,NULL);
#endif
	ignorance_level_add_rule(tmplvl,tmprule);
	g_string_free(tmpgs,TRUE);

	return TRUE;
}

gboolean save_conf()
{
	FILE *f;
	gchar *name, tempfilename[BUF_LONG];
	gint fd, i;

	name = g_build_filename(purple_user_dir(), "ignorance", NULL);
	strcpy(tempfilename, name);
	strcat(tempfilename,".XXXXXX");
	fd = g_mkstemp(tempfilename);
	if(fd<0) {
		perror(tempfilename);
		g_free(name);
		return FALSE;
	}
	if (!(f = fdopen(fd, "w"))) {
		perror("fdopen");
		close(fd);
		g_free(name);
		return FALSE;
	}

	fchmod(fd, S_IRUSR | S_IWUSR);

	for(i=0;i<levels->len;++i){
		ignorance_level_write(g_ptr_array_index(levels,i),f);
	}

	if(fclose(f)) {
		purple_debug_error("ignorance",
				   "Error writing to %s: %m\n", tempfilename);
		unlink(tempfilename);
		g_free(name);
		return FALSE;
	}
	rename(tempfilename, name);
	g_free(name);
	return TRUE;
}

ignorance_level* ignorance_get_level_name(const GString *levelname){
	gint i=0;
	ignorance_level *il=NULL;

	for(i=0;i<levels->len;++i){
		il=(ignorance_level*)g_ptr_array_index(levels,i);
		if(g_string_equal(levelname,il->name))
			return il;
	}

	return NULL;
}

gboolean ignorance_add_level(ignorance_level *level){
	gboolean rv=FALSE;

	if(level){
		g_ptr_array_add(levels,level);
		rv=TRUE;
	}

	return rv;
}

gboolean ignorance_remove_level(const GString *levelname){
	ignorance_level *level=ignorance_get_level_name(levelname);
	gboolean rv=FALSE;

	if(level){
		rv=g_ptr_array_remove(levels,level);
		ignorance_level_free(level);
	}

	return rv;
}

static void
ignorance_signals_connect(PurplePlugin *plugin)
{
	void *conv_handle, *blist_handle;

	conv_handle = purple_conversations_get_handle();
	blist_handle = purple_blist_get_handle();

	purple_signal_connect(conv_handle, "receiving-im-msg", plugin,
			PURPLE_CALLBACK (im_cb), NULL);
	purple_signal_connect(conv_handle, "receiving-chat-msg", plugin,
			PURPLE_CALLBACK (chat_cb), NULL);
	purple_signal_connect(conv_handle,"chat-buddy-joining", plugin,
			PURPLE_CALLBACK(chat_joinleave_cb),NULL);
	purple_signal_connect(conv_handle,"chat-buddy-leaving", plugin,
			PURPLE_CALLBACK(chat_joinleave_cb),NULL);
	purple_signal_connect(conv_handle,"chat-invited", plugin,
			PURPLE_CALLBACK(chat_invited_cb),NULL);
	purple_signal_connect(blist_handle,"buddy-added", plugin,
			PURPLE_CALLBACK(buddy_added_cb),NULL);
	purple_signal_connect(blist_handle,"buddy-removed", plugin,
			PURPLE_CALLBACK(buddy_removed_cb),NULL);

	return;
}

static gboolean load_conf() {
	gchar *buf, *ibuf;
	gint pnt = 0;
	gsize size;
	FILE *conffile = NULL;
	static ignorance_level *tmplvl = NULL;
	static ignorance_rule *tmprule = NULL;
	GString *tmpgs = NULL;

	buf = g_build_filename(purple_user_dir(), "ignorance", NULL);

	purple_debug_info("ignorance", "Attempting to load conf file %s\n",buf);

	levels = g_ptr_array_new();

	if(!(conffile = fopen(buf, "r"))) {
		g_free(buf);

		buf=g_build_filename(IGNORANCE_CONFDIR,"ignorance.conf",NULL);

		if(!(conffile = fopen(buf,"r"))) {
			purple_debug_info("ignorance",
					"Unable to open local or global conf files; falling back to defaults\n");
			generate_default_levels();
			import_purple_list();
			import_zinc_list();
			import_curphoo_list();

			g_free(buf);
			return FALSE;
		}
	}

	g_file_get_contents(buf, &ibuf, &size, NULL);
	fclose(conffile);
	g_free(buf);

	if(!ibuf) {
		generate_default_levels();
		import_purple_list();
		import_zinc_list();
		import_curphoo_list();
		return FALSE;
	}

	while(buf_get_line(ibuf, &buf, &pnt, size)) {
		if((*buf) == '#'){
			 /* ignore */
		} else if(strstr(buf, "level")){
			if('\0' == buf[5]){
				tmpgs = g_string_new("");

				for( ; buf != strstr(buf, "/level");
						buf_get_line(ibuf, &buf, &pnt, size))
				{
					g_string_append(tmpgs, buf);
					g_string_append(tmpgs, "\n");
				}

				tmplvl = ignorance_level_read(tmpgs->str);

				g_string_free(tmpgs, TRUE);
			} else
				tmplvl=ignorance_level_read(buf);

			if(tmplvl) {
	 			purple_debug_info("ignorance", "Adding level %s\n",
						tmplvl->name->str);

				ignorance_add_level(tmplvl);
			}
		 } else if(strstr(buf, "rule") && tmplvl) {
			if('\0' == buf[4]) {
				tmpgs = g_string_new("");

				for( ; buf != strstr(buf, "/rule");
						buf_get_line(ibuf, &buf, &pnt, size))
				{
					g_string_append(tmpgs, buf);
					g_string_append(tmpgs, "\n");
				}

		 		purple_debug_info("ignorance", "Attempting to read rule %s\n",
						tmpgs->str);

				tmprule = ignorance_rule_read(tmpgs->str);

				g_string_free(tmpgs, TRUE);
			} else
				 tmprule = ignorance_rule_read(buf);
			if(tmprule) {
	 			purple_debug_info("ignorance", "Adding rule %s: %s\n",
						tmprule->name->str, (gchar*)(tmprule->value));

		 		ignorance_level_add_rule(tmplvl, tmprule);
			}
		 } else if(tmplvl) {
			tmpgs = g_string_new(purple_normalize_nocase(NULL, buf));

	 		purple_debug_info("ignorance", "Adding denizen %s\n", buf);

			if(ignorance_get_user_level(tmpgs) == ignorance_get_default_level())
				ignorance_level_add_denizen(tmplvl, tmpgs);

			g_string_free(tmpgs,TRUE);
		}
	}

	g_free(ibuf);

	import_purple_list();
	import_zinc_list();
	import_curphoo_list();

	return TRUE;
}

static gboolean
ignorance_load (PurplePlugin *plugin) {
	purple_debug_info("ignorance", "Loading ignorance plugin");

	load_conf();

	ignorance_signals_connect(plugin);

	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin) {
	purple_debug_info("ignorance", "Unloading ignorance plugin\n");
	save_conf();

	return TRUE;
}

static GtkWidget *get_config_frame(PurplePlugin *plugin) {
	return create_uiinfo(levels);
}

static PidginPluginUiInfo ui_info = { get_config_frame };

static PurplePluginInfo ig_info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	PIDGIN_PLUGIN_TYPE,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,
	IGNORANCE_PLUGIN_ID,
	NULL,
	PP_VERSION,
	NULL,
	NULL,
	"Peter Lawler <bleeter from users.sf.net>, "
	"Levi Bard <taktaktaktaktaktaktaktaktaktak@gmail.com> (original author)",
	"http://guifications.sourceforge.net",
	ignorance_load,
	plugin_unload,
	NULL,
	&ui_info,
	NULL,
	NULL,
	NULL
};

static void 
ignorance_init (PurplePlugin * plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(PP_PACKAGE, PP_LOCALEDIR);
	bind_textdomain_codeset(PP_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	ig_info.name = _("Ignorance");
	ig_info.summary =
		_("Allows you to manage lists of users with various levels of allowable activity.");
	ig_info.description	=
		_("Allows you to manage lists of users with various levels of allowable activity.");
}

PURPLE_INIT_PLUGIN (ignorance, ignorance_init, ig_info)

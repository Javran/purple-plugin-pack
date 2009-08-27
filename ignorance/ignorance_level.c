/*
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301, USA.
 */

#include "ignorance.h"
#include "ignorance_denizen.h"
#include "ignorance_internal.h"
#include "ignorance_level.h"
#include "ignorance_violation.h"
#include "regex.h"

#include <string.h>
#include <strings.h>

void ignorance_level_write_hashitem(gpointer key, gpointer value,
									gpointer user_data);
void ignorance_hash_free(gpointer key, gpointer value,
						 gpointer user_data);
void ignorance_level_regex_hashitem(gpointer key, gpointer value,
									gpointer user_data);

gboolean assign_level_token(ignorance_level *lvl,const gchar *tokentxt);
gint g_string_compare(gconstpointer a, gconstpointer z);

ignorance_level* ignorance_level_new() {
	ignorance_level *il=(ignorance_level*)g_malloc(sizeof(ignorance_level));
	il->name=g_string_new("Default");
	il->denizens_hash=g_hash_table_new(g_str_hash,g_str_equal);
	il->rules=g_ptr_array_new();

	return il;
}

void ignorance_level_free(ignorance_level *il) {
	g_string_free(il->name,TRUE);
	g_hash_table_foreach(il->denizens_hash,ignorance_hash_free,NULL);
	g_hash_table_destroy(il->denizens_hash);
	g_ptr_array_foreach(il->rules,ignorance_rule_free_g,NULL);
}

void ignorance_level_free_g(gpointer il,gpointer user_data) {
	ignorance_level_free((ignorance_level*)il);
}

gboolean ignorance_level_add_rule(ignorance_level *level,ignorance_rule *rule) {
	g_ptr_array_add(level->rules,(gpointer)rule);

	return TRUE;
}

ignorance_rule* ignorance_level_get_rule(ignorance_level *level,
										 const GString *rulename) {

	int i=0;
	ignorance_rule *rule=NULL;

	for(i=0;i<level->rules->len;++i){
		rule=g_ptr_array_index(level->rules,i);
		if(g_string_equal(rulename,rule->name))
			return g_ptr_array_index(level->rules,i);
	}

	return NULL;
}

gboolean ignorance_level_remove_rule(ignorance_level *level,
									 const GString *rulename) {
	return g_ptr_array_remove_fast(level->rules,
								   ignorance_level_get_rule(level,rulename));
}

gboolean ignorance_level_add_denizen(ignorance_level *level,
									 const GString *username) {

	if(!g_hash_table_lookup(level->denizens_hash,username->str)){
		ignorance_denizen *id=ignorance_denizen_new(username->str);
		g_hash_table_insert(level->denizens_hash,
							ignorance_denizen_get_name(id),id);
	}

	return TRUE;
}

#ifdef HAVE_REGEX_H
gboolean ignorance_level_has_denizen_regex(ignorance_level *level,
										   const gchar *regex, GList **denizens) {

	regex_t reg;
	gpointer udata[2];

	udata[0]=denizens;

	if(regcomp(&reg,regex,REG_EXTENDED | REG_NOSUB)) {
		purple_debug_error("ignorance", "Error parsing regex %s\n",
				   regex);
		regfree(&reg);
		return FALSE;
	}

	udata[1]=&reg;

	g_hash_table_foreach(level->denizens_hash,ignorance_level_regex_hashitem,
						 (gpointer)udata);

	regfree(&reg);

	return (denizens!=NULL);
}
#endif


gboolean ignorance_level_has_denizen(ignorance_level *level,
									 const GString *username) {
	gboolean rv=FALSE;

	rv = (NULL != g_hash_table_lookup(level->denizens_hash,username->str));

	return rv;
}

gint g_string_compare(gconstpointer a, gconstpointer z) {
	const GString *gsa=(const GString*)a, *gsz=(const GString*)z;

	return g_ascii_strcasecmp(gsa->str, gsz->str);
}


gboolean ignorance_level_remove_denizen(ignorance_level *level,
										const GString *username) {

	gpointer kptr=NULL, vptr=NULL;
	gboolean rv=FALSE;

	rv=g_hash_table_lookup_extended(level->denizens_hash, username->str, &kptr,
									&vptr);

	purple_debug_info("ignorance","Remove: found id %x\n",vptr);
	if(rv){
		purple_debug_info("ignorance","Removing from hash\n");
		g_hash_table_remove(level->denizens_hash,username->str);

		purple_debug_info("ignorance","Freeing denizen\n");
		ignorance_denizen_free((ignorance_denizen*)vptr);
		purple_debug_info("ignorance","Done freeing denizen\n");
	}

	return rv;
}

gint ignorance_level_rulecheck(ignorance_level *level,
							   const GString *username, const GString *text,
							   gint flags, GList **violations) {
	int i=0, totalscore=0, curscore;
	ignorance_rule *cur;
	ignorance_denizen *id;

	purple_debug_info("ignorance","Preparing to lookup %s\n", username->str);
	id = g_hash_table_lookup(level->denizens_hash, username->str);
	purple_debug_info("ignorance","Got denizen %x\n",id);
	if(id){
		purple_debug_info("ignorance","Making sure text isn't name\n");
		if(strcasecmp(ignorance_denizen_get_name(id),text->str)){
			purple_debug_info("ignorance","Setting new message to %s\n", text->str);
			ignorance_denizen_set_message(id, text->str);
		}
	}

	for(i=0;i<level->rules->len;++i) {
		cur=(ignorance_rule*)g_ptr_array_index(level->rules,i);
		if(cur->flags & IGNORANCE_APPLY_USER) {
			curscore=ignorance_rule_rulecheck(cur,username,flags);
			totalscore|=curscore;
			if(curscore){
				if(curscore & IGNORANCE_FLAG_MESSAGE)
					(*violations)=g_list_prepend(*violations,
								ignorance_violation_newp(IGNORANCE_FLAG_MESSAGE,
								cur->message));
				if(curscore & IGNORANCE_FLAG_SOUND)
					(*violations)=g_list_prepend(*violations,
								ignorance_violation_newp(IGNORANCE_FLAG_SOUND,
								cur->sound));
				if(curscore & IGNORANCE_FLAG_EXECUTE)
					(*violations)=g_list_prepend(*violations,
								ignorance_violation_newp(IGNORANCE_FLAG_EXECUTE,
								cur->command));
			}
		}

		curscore=ignorance_rule_rulecheck(cur, text,
										  flags & (~IGNORANCE_APPLY_USER));

		totalscore|=curscore;
		if(curscore){
			if(curscore & IGNORANCE_FLAG_MESSAGE)
				(*violations)=g_list_prepend(*violations, 
					ignorance_violation_newp(IGNORANCE_FLAG_MESSAGE, cur->message));
			if(curscore & IGNORANCE_FLAG_SOUND)
				(*violations)=g_list_prepend(*violations,
					ignorance_violation_newp(IGNORANCE_FLAG_SOUND, cur->sound));
			if(curscore & IGNORANCE_FLAG_EXECUTE)
				(*violations)=g_list_prepend(*violations,
					ignorance_violation_newp(IGNORANCE_FLAG_EXECUTE, cur->command));
		}
	}

	return totalscore;
}


ignorance_level* ignorance_level_read_old(const gchar *lvltext) {
	gchar *tokptr=strchr((gchar*)lvltext,' '), **tokens=NULL;
	ignorance_level *lvl=ignorance_level_new();
	int i=0;

	if(!tokptr){
		ignorance_level_free(lvl);
		return NULL;
	}

	tokens=g_strsplit(lvltext," ",INT_MAX);

	for(i=0;tokens[i];++i)
		assign_level_token(lvl,tokens[i]);

	g_strfreev(tokens);

	return lvl;
}

ignorance_level* ignorance_level_read(const gchar *lvltext) {
	gchar *tokptr=strchr((gchar*)lvltext,'\n'), **tokens=NULL;
	ignorance_level *lvl=ignorance_level_new();
	int i=0;

	if(!tokptr){
		ignorance_level_free(lvl);
		return ignorance_level_read_old(lvltext);
	}
	tokens=g_strsplit(lvltext,"\n",INT_MAX);
	for(i=0;tokens[i];++i)
		assign_level_token(lvl,tokens[i]);

	g_strfreev(tokens);

	return lvl;
}


/* Parses out a token of the form tokenname="value" and assigns it to a rulename
 *
 * rule is the rule to be updated
 * tokentxt is the token string
 * true returned if token is valid and successfully added to the rule
 *
 * level name1="value1" name2="value2" ...
 */
gboolean assign_level_token(ignorance_level *lvl,const gchar *tokentxt) {
	gchar *name=NULL, *value=NULL;
	gboolean rv=TRUE;
	gint cursor=0;

		value=strchr(tokentxt,'=');
		if(value) {
			(*value)='\0';
			++value;

			if('"'==(*value)) {
				++value;
				cursor=strlen(value)-1;
				if('"'==value[cursor])
					value[cursor]='\0';
			}
			name=(gchar*)tokentxt;

			if(!strncasecmp(name,"name",BUFSIZ))
				g_string_assign(lvl->name,value);
			else
				rv=FALSE;
		} else
			rv=FALSE;

		return rv;
}

gboolean ignorance_level_write(ignorance_level *level,FILE *f) {
	gint i;

	fprintf(f,"level\nname=\"%s\"\n/level\n", level->name->str);

	for(i=0;i<level->rules->len;++i)
		ignorance_rule_write(g_ptr_array_index(level->rules,i),f);

	g_hash_table_foreach(level->denizens_hash,ignorance_level_write_hashitem,f);

	return TRUE;
}

void ignorance_level_regex_hashitem(gpointer key, gpointer value,
									gpointer user_data) {
	gpointer *udata=(gpointer*)user_data;
	GList **denizens=(GList**)udata[0];

	if(!(regexec((regex_t*)udata[1],(gchar*)key,1,NULL,0)))
		(*denizens)=g_list_prepend(*denizens,g_string_new((gchar*)key));
}

void ignorance_hash_free(gpointer key, gpointer value, gpointer user_data) {
	ignorance_denizen_free((ignorance_denizen*)value);
}

void ignorance_level_write_hashitem(gpointer key, gpointer value,
									gpointer user_data){

	fprintf((FILE*)user_data,"%s\n",(gchar*)key);

}

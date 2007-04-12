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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "ignorance_rule.h"
#include <string.h>
#include <strings.h>
#include "ignorance_internal.h"


gboolean assign_rule_token(ignorance_rule *rule, const char *tokentxt);

ignorance_rule* ignorance_rule_new() {
	ignorance_rule *ir=(ignorance_rule*)g_malloc(sizeof(ignorance_rule));

	ir->name=g_string_new("");
	ir->type=IGNORANCE_RULE_SIMPLETEXT;
	ir->score=0;
	ir->flags=0;
	ir->enabled=TRUE;
	ir->value=NULL;
	ir->message=NULL;
	ir->sound=NULL;
	ir->command=NULL;

	return ir;
}

ignorance_rule* ignorance_rule_newp(const GString *name, gint type,
									const gchar *value, gint score, gint flags,
									gboolean enabled, const gchar *message,
									const gchar *sound, const gchar *command) {
	ignorance_rule *ir=(ignorance_rule*)g_malloc(sizeof(ignorance_rule));

	ir->name=g_string_new(name->str);
	if(ignorance_rule_has_type(type))
		ir->type=type;
	else
		ir->type=IGNORANCE_RULE_INVALID;
	ir->value=g_strdup(value);
	ir->score=score;
	ir->flags=flags;
	ir->enabled=enabled;
	ir->message=g_strdup(message);
	ir->sound=g_strdup(sound);
	ir->command=g_strdup(command);

	return ir;
}

void ignorance_rule_free(ignorance_rule *ir) {
	g_string_free(ir->name,TRUE);
	g_free(ir->value);
	g_free(ir->message);
	g_free(ir->sound);
	g_free(ir->command);
	g_free(ir);
}

void ignorance_rule_free_g(gpointer ir,gpointer user_data) {
	ignorance_rule_free((ignorance_rule*)ir);
}

gboolean ignorance_rule_has_type(gint type) {
	if((type>=IGNORANCE_RULE_MINVALID) || (type<=IGNORANCE_RULE_INVALID))
		return FALSE;
	return TRUE;
}


gint ignorance_rule_rulecheck(ignorance_rule *rule, const GString *text,
							  gint flags) {
	if((flags & rule->flags) && rule->enabled){
		switch(rule->type){
			case IGNORANCE_RULE_SIMPLETEXT:
				return simple_text_rulecheck(rule,text);
#ifdef HAVE_REGEX_H
			case IGNORANCE_RULE_REGEX:
				return regex_rulecheck(rule,text);
#endif
			default:
				return 0;
		}
	}

	return 0;
}

gint simple_text_rulecheck(ignorance_rule *rule,const GString *text) {
	const gchar *rulevalue=(const gchar*)(rule->value);

	if(NULL!=g_strstr_len(text->str,text->len,rulevalue))
		return rule->score;

	return 0;
}

#ifdef HAVE_REGEX_H
gint regex_rulecheck(ignorance_rule *rule, const GString *text) {
	regex_t	reg;
	gint rv=0;
	
	if(regcomp(&reg,(const gchar*)rule->value,REG_EXTENDED | REG_NOSUB))
		purple_debug_error("ignorance", "Error parsing regex %s\n",
				   (const gchar*)(rule->value));
	else if(!regexec(&reg,text->str,1,NULL,0))
		rv=rule->score;

	regfree(&reg);
	return rv;
}
#endif

gint repeat_rulecheck(ignorance_rule *rule, gint repeats) {
	gint allowed_repeats = atoi((gchar*)(rule->value));
	gint score=0;

	if(repeats >= allowed_repeats)
		score=rule->score;

	return score;
}

ignorance_rule* ignorance_rule_read_old(const gchar *ruletext) {
	gchar *tokptr=strchr((gchar*)ruletext,' '), **tokens=NULL;
	ignorance_rule *rule=ignorance_rule_new();
	int i=0;

	if(!tokptr){
		ignorance_rule_free(rule);
		return NULL;
	}

	tokens=g_strsplit(ruletext," ",INT_MAX);

	for(i=0;tokens[i];++i)
		assign_rule_token(rule,tokens[i]);

	if(rule->score > 9 || rule->score < -9)
		rule->score=IGNORANCE_FLAG_IGNORE;
	else
		rule->score=IGNORANCE_FLAG_FILTER;

	g_strfreev(tokens);

	return rule;
}

ignorance_rule* ignorance_rule_read(const gchar *ruletext) {
	gchar*tokptr=strchr((gchar*)ruletext,'\n'), **tokens;
	ignorance_rule *rule=ignorance_rule_new();
	int i=0;

	if(!tokptr){
		ignorance_rule_free(rule);
		return ignorance_rule_read_old(ruletext);
	}

	tokens=g_strsplit(ruletext,"\n",INT_MAX);

	for(i=0;tokens[i];++i)
		assign_rule_token(rule,tokens[i]);

	g_strfreev(tokens);

	return rule;
}

/*
 * Parses out a token of the form tokenname="value" and assigns it to a rulename
 *
 * rule is the rule to be updated
 * tokentxt is the token string
 * true returned if token is valid and successfully added to the rule
 *
 * level name1="value1" name2="value2" ...
 */
gboolean assign_rule_token(ignorance_rule *rule,const gchar *tokentxt) {
	gchar *name=(gchar*)tokentxt, *value=NULL;
	gboolean rv=TRUE;
	gint cursor=0;

		value=strchr(tokentxt,'=');
		if(value) {
			(*value)='\0';
			++value;

			if('"'==(*value)){
				++value;
				cursor=strlen(value)-1;
				if('"'==value[cursor])
					value[cursor]='\0';
			}

			if(!strncasecmp(name,"name",BUFSIZ))
				g_string_assign(rule->name,value);
			else if(!strncasecmp(name,"type",BUFSIZ))
				rule->type=atoi(value);
			else if(!strncasecmp(name,"value",BUFSIZ)) {
				rule->value=(gchar*)g_malloc((strlen(value)+1)*sizeof(gchar));
				strncpy(rule->value,value,strlen(value)+1);
			}else if(!strncasecmp(name,"score",BUFSIZ))
				rule->score=atoi(value);
			else if(!strncasecmp(name,"flags",BUFSIZ))
				rule->flags=atoi(value);
			else if(!strncasecmp(name,"enabled",BUFSIZ))
				rule->enabled=(gboolean)atoi(value);
			else if(!strncasecmp(name,"message",BUFSIZ)) {
				rule->message=(gchar*)g_malloc((strlen(value)+1)*sizeof(gchar));
				strncpy(rule->message,value,strlen(value)+1);
			} else if(!strncasecmp(name,"command",BUFSIZ)) {
				rule->command=(gchar*)g_malloc((strlen(value)+1)*sizeof(gchar));
				strncpy(rule->command,value,strlen(value)+1);
			} else if(!strncasecmp(name,"sound",BUFSIZ)) {
				rule->sound=(gchar*)g_malloc((strlen(value)+1)*sizeof(gchar));
				strncpy(rule->sound,value,strlen(value)+1);
			} else
				rv=FALSE;
		} else
			rv=FALSE;

		return rv;
}

gboolean ignorance_rule_write(ignorance_rule *rule,FILE *f){
	fprintf(f,"rule\nname=\"%s\"\ntype=\"%d\"\nscore=\"%d\"\nvalue=\"%s\"\nflags=\"%d\"\nenabled=\"%d\"\nmessage=\"%s\"\ncommand=\"%s\"\nsound=\"%s\"\n/rule\n",
			rule->name->str, rule->type, rule->score, (gchar*)(rule->value),
			rule->flags, rule->enabled, rule->message, rule->command, rule->sound);

	return TRUE;
}

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

#include <string.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "ignorance.h"
#include "ignorance_internal.h"

static gboolean populate_panel(GtkTreeSelection *sel);
static gboolean verify_form();
static gboolean add_rule_from_form(GtkTreeView *view);
static gboolean add_group_from_form(GtkTreeView *view);
static gboolean edit_rule_from_form(GtkTreeView *view);
static gboolean del_rule_from_form(GtkTreeView *view);
static gboolean del_group_from_form(GtkTreeView *view);
static gboolean ignorance_rule_valid(const gchar *ruletext, gint ruletype);
static gboolean ignorance_rulename_valid(const gchar *rule_name);

gboolean on_levelView_row_activated (GtkTreeSelection *sel, gpointer user_data) {
	populate_panel(sel);
	return FALSE;
}

void on_levelAdd_clicked (GtkButton *button, gpointer user_data) {

	if(verify_form()) {
		if(!add_rule_from_form(user_data)) {
			/* XXX: */
		}
	}
}

void on_groupAdd_clicked (GtkButton *button, gpointer user_data) {
	if(verify_form()) {
		if(!add_group_from_form(user_data)) {
			/* XXX: Do something I guess? -- sadrul */
		}
	}
}

void on_levelEdit_clicked (GtkButton *button, gpointer user_data) {
	if(verify_form()) {
		if(!edit_rule_from_form(user_data)) {
			/* XXX: Do something I guess? -- sadrul */
		}
	}
}

void
on_levelDel_clicked (GtkButton *button, gpointer user_data) {
	if(rule_selected) {
		if(verify_form()) {
			if(!del_rule_from_form(user_data)) {
				/* XXX: */
			}
		}
	} else {
		del_group_from_form(user_data);
	}
}

static gboolean populate_panel(GtkTreeSelection *sel) {
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *levnames, *rulenames;
	ignorance_level	*level=NULL;
	ignorance_rule	*rule=NULL;
	GString *levgs, *rulegs;

	if (gtk_tree_selection_get_selected (sel, &model, &iter)) {
			gtk_tree_model_get (model, &iter, LEVEL_COLUMN, &levnames, -1);
			gtk_tree_model_get(model,&iter,RULE_COLUMN,&rulenames,-1);
		if(strlen(rulenames)) {
			rule_selected=TRUE;
			levgs=g_string_new(levnames);
			rulegs=g_string_new(rulenames);
			level=ignorance_get_level_name(levgs);
			rule=ignorance_level_get_rule(level,rulegs);
			if(!rule) {
				fprintf(stderr,"Ignorance: Unable to find rule %s on level %s\n",
						rulegs->str,level->name->str);
				return FALSE;
			}

			gtk_entry_set_text(GTK_ENTRY(rulename),rulenames);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(filter_cb),
										 rule->score & IGNORANCE_FLAG_FILTER);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ignore_cb),
										 rule->score & IGNORANCE_FLAG_IGNORE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(message_cb),
										 rule->score & IGNORANCE_FLAG_MESSAGE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sound_cb),
										 rule->score & IGNORANCE_FLAG_SOUND);

			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(execute_cb),
										 rule->score & IGNORANCE_FLAG_EXECUTE);

			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(message_cb))) {
				gtk_entry_set_text(GTK_ENTRY(message_entry),rule->message);
			}

			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sound_cb))) {
				gtk_entry_set_text(GTK_ENTRY(sound_entry),rule->sound);
			}

			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(execute_cb))) {
				gtk_entry_set_text(GTK_ENTRY(execute_entry),rule->command);
			}

			gtk_entry_set_text(GTK_ENTRY(filtervalue),
							   (const gchar*)(rule->value));
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(im_type_cb),
										 rule->flags & IGNORANCE_APPLY_IM);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chat_type_cb),
										 rule->flags & IGNORANCE_APPLY_CHAT);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(username_type_cb),
										 rule->flags & IGNORANCE_APPLY_USER);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enterleave_type_cb),
										 rule->flags & IGNORANCE_APPLY_ENTERLEAVE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(invite_type_cb),
										 rule->flags & IGNORANCE_APPLY_INVITE);


#ifdef HAVE_REGEX_H
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(regex_cb),
										 rule->type & IGNORANCE_RULE_REGEX);
#endif

			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(repeat_cb),
										 rule->type & IGNORANCE_RULE_REPEAT);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enabled_cb),
										 rule->enabled);

			gtk_button_set_label(GTK_BUTTON(levelDel),"Remove rule");

			g_string_free(levgs,TRUE);
			g_string_free(rulegs,TRUE);
		} else {
			rule_selected=FALSE;
			gtk_entry_set_text(GTK_ENTRY(rulename),"");
			gtk_entry_set_text(GTK_ENTRY(filtervalue),"");

			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(im_type_cb),FALSE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chat_type_cb),FALSE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(username_type_cb),
										 FALSE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enterleave_type_cb),
										 FALSE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(invite_type_cb),
										 FALSE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(filter_cb),FALSE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ignore_cb),FALSE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(message_cb),FALSE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sound_cb),FALSE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(execute_cb),FALSE);

#ifdef HAVE_REGEX_H
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(regex_cb),FALSE);
#endif
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(repeat_cb),FALSE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enabled_cb),FALSE);

			gtk_button_set_label(GTK_BUTTON(levelDel),"Remove group");
		}

		g_free(levnames);
		g_free(rulenames);
	}

	return FALSE;
}

void on_filter_cb_toggled  (GtkButton *button, gpointer user_data) {
	gboolean amdisabled=!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

	if(!amdisabled) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(message_cb),amdisabled);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ignore_cb),amdisabled);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sound_cb),amdisabled);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(execute_cb),amdisabled);
	}

	/* Maybe we *should* allow them to send a message on filter */
	gtk_widget_set_sensitive(message_cb,amdisabled);
	gtk_widget_set_sensitive(ignore_cb,amdisabled);
	gtk_widget_set_sensitive(sound_cb,amdisabled);
	gtk_widget_set_sensitive(execute_cb,amdisabled);
}

void on_ignore_cb_toggled (GtkButton *button, gpointer user_data) {
	gboolean amdisabled=!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

	if(!amdisabled) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(message_cb),amdisabled);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(filter_cb),amdisabled);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sound_cb),amdisabled);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(execute_cb),amdisabled);
	}

	/* Maybe we *should* allow them to send a message on ignore */
	gtk_widget_set_sensitive(message_cb,amdisabled);
	gtk_widget_set_sensitive(filter_cb,amdisabled);
	gtk_widget_set_sensitive(sound_cb,amdisabled);
	gtk_widget_set_sensitive(execute_cb,amdisabled);
}

void on_message_cb_toggled (GtkButton *button, gpointer user_data) {
		gboolean isactive=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(message_cb));
		if(!isactive) {
				gtk_entry_set_text(GTK_ENTRY(message_entry),"");
		}
		gtk_widget_set_sensitive(message_entry,isactive);
}

void on_sound_cb_toggled (GtkButton *button, gpointer user_data) {
		gboolean isactive=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sound_cb));

		if(!isactive) {
				gtk_entry_set_text(GTK_ENTRY(sound_entry),"");
		}
		gtk_widget_set_sensitive(sound_entry,isactive);
#if GTK_CHECK_VERSION(2,4,0)
		gtk_widget_set_sensitive(sound_browse,isactive);
#endif
}

void on_execute_cb_toggled (GtkButton *button, gpointer user_data) {
		gboolean isactive=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(execute_cb));
		if(!isactive) {
				gtk_entry_set_text(GTK_ENTRY(execute_entry),"");
		}
		gtk_widget_set_sensitive(execute_entry,isactive);
		/*gtk_widget_set_sensitive(execute_browse,isactive);*/
}

void on_sound_browse_clicked (GtkButton *button, gpointer user_data) {
#if GTK_CHECK_VERSION(2,4,0)
		GtkWidget *fcd=gtk_file_chooser_dialog_new("Choose sound",NULL,
					   GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL,
					   GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					   NULL);
		gchar *filename=NULL;

		if(GTK_RESPONSE_ACCEPT == gtk_dialog_run(GTK_DIALOG(fcd))) {
				filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fcd));
				if(filename) {
					gtk_entry_set_text(GTK_ENTRY(sound_entry),filename);
					g_free(filename);
				}
		}

		gtk_widget_destroy(fcd);
#endif
}

/*No command browser for now
void on_execute_browse_clicked (GtkButton *button, gpointer user_data) {
		gchar *filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(execute_browse));
		if(filename) {
				gtk_entry_set_text(GTK_ENTRY(execute_entry),filename);
		}
}
*/

static gboolean verify_form() {
	gboolean rv=TRUE;
	gchar *tmp;
	gint type=IGNORANCE_RULE_SIMPLETEXT;
 
	tmp=(gchar*)gtk_entry_get_text(GTK_ENTRY(filtervalue));
#ifdef HAVE_REGEX_H
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(regex_cb))) {
		type=IGNORANCE_RULE_REGEX;
	}
#endif
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(repeat_cb))) {
		type=IGNORANCE_RULE_REPEAT;
	}

	rv=ignorance_rule_valid(tmp,type);

	if(!rv) {
		purple_debug_error("ignorance","Rule invalid: %s\n",tmp);
	}else{
		tmp=(gchar*)gtk_entry_get_text(GTK_ENTRY(rulename));
		rv=ignorance_rulename_valid(tmp);

		if(!rv) {
			purple_debug_error("ignorance","Rule name invalid: %s\n",tmp);
		}
	}

	return rv;
}


static gboolean add_rule_from_form(GtkTreeView *view) {
	ignorance_rule *rule=NULL;
	ignorance_level *level=NULL;
	GString	*rule_name, *level_name;
	GtkTreeSelection *sel;
	GtkTreeIter iter, childiter;
	GtkTreeModel *model;
	gchar *rule_value=NULL, *rule_message=NULL, *rule_sound=NULL,
		  *rule_command=NULL;
	gint rule_score=0, rule_type=IGNORANCE_RULE_SIMPLETEXT, rule_flags=0;
	gboolean rv=FALSE, rule_enable;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));

	if (!gtk_tree_selection_get_selected (sel, &model, &iter)) {
		return rv;
	}

	gtk_tree_model_get (model, &iter, LEVEL_COLUMN, &rule_value, -1);

	if(!rule_value) {
		return rv;
	}else if(!strlen(rule_value)) {
		g_free(rule_value);
		return rv;
	}

	level_name=g_string_new(rule_value);
	g_free(rule_value);

	gtk_tree_model_get(model,&iter,RULE_COLUMN,&rule_value,-1);
	if(strlen(rule_value)) {
		childiter=iter;
		gtk_tree_model_iter_parent(model,&iter,&childiter);
	}

	g_free(rule_value);

	level=ignorance_get_level_name(level_name);

	rule_name=g_string_new(gtk_entry_get_text(GTK_ENTRY(rulename)));

	rule=ignorance_level_get_rule(level,rule_name);

	if(rule) {
		g_string_free(level_name,TRUE);
		g_string_free(rule_name,TRUE);
		return rv;
	}

#ifdef HAVE_REGEX_H
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(regex_cb))) {
		rule_type=IGNORANCE_RULE_REGEX;
	}
#endif
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(repeat_cb))) {
		rule_type=IGNORANCE_RULE_REPEAT;
	}

	rule_value=(gchar*)gtk_entry_get_text(GTK_ENTRY(filtervalue));

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(im_type_cb))) {
		rule_flags |= IGNORANCE_APPLY_IM;
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chat_type_cb))) {
		rule_flags |= IGNORANCE_APPLY_CHAT;
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(username_type_cb))) {
		rule_flags |= IGNORANCE_APPLY_USER;
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(enterleave_type_cb))) {
		rule_flags |= IGNORANCE_APPLY_ENTERLEAVE;
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(invite_type_cb))) {
		rule_flags |= IGNORANCE_APPLY_INVITE;
	}

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(filter_cb))) {
			rule_score |= IGNORANCE_FLAG_FILTER;
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ignore_cb))) {
			rule_score |= IGNORANCE_FLAG_IGNORE;
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(message_cb))) {
			rule_score |= IGNORANCE_FLAG_MESSAGE;
			rule_message=(gchar*)gtk_entry_get_text(GTK_ENTRY(message_entry));
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sound_cb))) {
			rule_score |= IGNORANCE_FLAG_SOUND;
			rule_sound=(gchar*)gtk_entry_get_text(GTK_ENTRY(sound_entry));
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(execute_cb))) {
			rule_score |= IGNORANCE_FLAG_EXECUTE;
			rule_command=(gchar*)gtk_entry_get_text(GTK_ENTRY(execute_entry));
	}

	rule_enable=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(enabled_cb));

	rule=ignorance_rule_newp(rule_name,rule_type,rule_value,rule_score,
							 rule_flags,rule_enable,rule_message,rule_sound,
							 rule_command);

	rv=ignorance_level_add_rule(level,rule);

	if(rv) {
		gtk_tree_store_append(GTK_TREE_STORE(model),&childiter,&iter);
		gtk_tree_store_set(GTK_TREE_STORE(model),&childiter,LEVEL_COLUMN,level_name->str,RULE_COLUMN,rule_name->str,-1);
	}

	g_string_free(level_name,TRUE);
	g_string_free(rule_name,TRUE);

	return rv;
}

static gboolean add_group_from_form(GtkTreeView *view) {
	gboolean rv=TRUE;
	GtkTreeIter iter;
	GtkTreeStore *store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(view)));

	ignorance_level *level=ignorance_level_new();
	level->name=g_string_new(gtk_entry_get_text(GTK_ENTRY(rulename)));

	rv=ignorance_add_level(level);

	if(rv) {
		gtk_tree_store_append(store,&iter,NULL);
		gtk_tree_store_set (store, &iter, LEVEL_COLUMN, level->name->str,
							RULE_COLUMN, "", -1);
	}

	return rv;
}

static gboolean edit_rule_from_form(GtkTreeView *view) {
	ignorance_level *level;
	ignorance_rule *rule;
	GString *rule_name, *level_name;
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *rule_value, *rule_message=NULL, *rule_sound=NULL, *rule_command=NULL;
	gint rule_score=0, rule_type=IGNORANCE_RULE_SIMPLETEXT, rule_flags=0;
	gboolean rule_enable, rv;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));

	if (!gtk_tree_selection_get_selected (sel, &model, &iter)) {
		return FALSE;
	}

	gtk_tree_model_get (model, &iter, LEVEL_COLUMN, &rule_value, -1);

	if(!rule_value) {
		return FALSE;
	}else if(!strlen(rule_value)) {
		g_free(rule_value);
		return FALSE;
	}

	level_name=g_string_new(rule_value);
	g_free(rule_value);
	level=ignorance_get_level_name(level_name);

	rule_name=g_string_new(gtk_entry_get_text(GTK_ENTRY(rulename)));

	rule=ignorance_level_get_rule(level,rule_name);

	if(!rule) {
		fprintf(stderr,"Ignorance: Rule \"%s\" not found on level %s\n",rule_name->str,level_name->str);
		g_string_free(rule_name,TRUE);
		g_string_free(level_name,TRUE);
		return FALSE;
	}

#ifdef HAVE_REGEX_H
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(regex_cb))) {
		rule_type=IGNORANCE_RULE_REGEX;
	}
#endif
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(repeat_cb))) {
		rule_type=IGNORANCE_RULE_REPEAT;
	}

	rule->type=rule_type;

	rule_value=(gchar*)gtk_entry_get_text(GTK_ENTRY(filtervalue));
	g_free(rule->value);
	rule->value=g_strdup(rule_value);

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(im_type_cb))) {
		rule_flags |= IGNORANCE_APPLY_IM;
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chat_type_cb))) {
		rule_flags |= IGNORANCE_APPLY_CHAT;
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(username_type_cb))) {
		rule_flags |= IGNORANCE_APPLY_USER;
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(enterleave_type_cb))) {
		rule_flags |= IGNORANCE_APPLY_ENTERLEAVE;
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(invite_type_cb))) {
		rule_flags |= IGNORANCE_APPLY_INVITE;
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(filter_cb))) {
			rule_score |= IGNORANCE_FLAG_FILTER;
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ignore_cb))) {
			rule_score |= IGNORANCE_FLAG_IGNORE;
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(message_cb))) {
			rule_score |= IGNORANCE_FLAG_MESSAGE;
			rule_message=(gchar*)gtk_entry_get_text(GTK_ENTRY(message_entry));
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sound_cb))) {
			rule_score |= IGNORANCE_FLAG_SOUND;
			rule_sound=(gchar*)gtk_entry_get_text(GTK_ENTRY(sound_entry));
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(execute_cb))) {
			rule_score |= IGNORANCE_FLAG_EXECUTE;
			rule_command=(gchar*)gtk_entry_get_text(GTK_ENTRY(execute_entry));
	}

	rule_enable=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(enabled_cb));

	rule=ignorance_rule_newp(rule_name, rule_type, rule_value, rule_score,
							 rule_flags,rule_enable,rule_message,rule_sound,
							 rule_command);

	rv=ignorance_level_remove_rule(level,rule_name);

	if(rv) {
		rv=ignorance_level_add_rule(level,rule);
	}

	rule->flags=rule_flags;
	rule->enabled=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(enabled_cb));

	g_string_free(rule_name,TRUE);
	g_string_free(level_name,TRUE);

	return rv;
}

static gboolean del_rule_from_form(GtkTreeView *view) {
	ignorance_level *level;
	ignorance_rule *rule;
	GString *rule_name, *level_name;
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *rule_value;
	gboolean rv=FALSE;
	
	sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	
	if (!gtk_tree_selection_get_selected (sel, &model, &iter)) {
		return rv;
	}

	gtk_tree_model_get (model, &iter, LEVEL_COLUMN, &rule_value, -1);

	if(!rule_value) {
		return rv;
	}else if(!strlen(rule_value)) {
		g_free(rule_value);
		return rv;
	}

	level_name=g_string_new(rule_value);
	g_free(rule_value);
	level=ignorance_get_level_name(level_name);

	rule_name=g_string_new(gtk_entry_get_text(GTK_ENTRY(rulename)));

	rule=ignorance_level_get_rule(level,rule_name);

	if(!rule) {
		fprintf(stderr,"Ignorance: Rule \"%s\" not found on level %s\n", 
				rule_name->str,level_name->str);
		g_string_free(rule_name,TRUE);
		g_string_free(level_name,TRUE);
		return rv;
	}

	rv=ignorance_level_remove_rule(level,rule_name);

	if(rv) {
		gtk_tree_store_remove(GTK_TREE_STORE(model),&iter);
	}

	g_string_free(rule_name,TRUE);
	g_string_free(level_name,TRUE);

	return rv;
}

static gboolean del_group_from_form(GtkTreeView *view) {
	GString *level_name;
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar *rule_value;
	gboolean rv=FALSE;

	sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(view));

	if (!gtk_tree_selection_get_selected (sel, &model, &iter)) {
		return rv;
	}

	gtk_tree_model_get (model, &iter, LEVEL_COLUMN, &rule_value, -1);

	if(!rule_value) {
		return rv;
	}else if(!strlen(rule_value)) {
		g_free(rule_value);
		return rv;
	}

	level_name=g_string_new(rule_value);
	g_free(rule_value);

	ignorance_remove_level(level_name);
	gtk_tree_store_remove(GTK_TREE_STORE(model),&iter);
	rv=TRUE;

	return rv;
}



gboolean load_form_with_levels(GtkTreeView *tree, GPtrArray *levels) {
	int i,j;
	GtkTreeStore *store=GTK_TREE_STORE(gtk_tree_view_get_model(tree));
	GtkTreeIter	 iter, ruleiter;
	ignorance_level *level;
	ignorance_rule *rule;

	if(!levels) {
		return FALSE;
	}

	for(i=0;i<levels->len;++i) {
		level=(ignorance_level*)g_ptr_array_index(levels,i);
		gtk_tree_store_append(store,&iter,NULL);
		gtk_tree_store_set (store, &iter, LEVEL_COLUMN, level->name->str,
							RULE_COLUMN, "", -1);

		for(j=0;j<level->rules->len;++j) {
			rule=(ignorance_rule*)g_ptr_array_index(level->rules,j);
			gtk_tree_store_append(store,&ruleiter,&iter);
			gtk_tree_store_set(store,&ruleiter, LEVEL_COLUMN, level->name->str,
							   RULE_COLUMN, rule->name->str, -1);
		}
	}

	return FALSE;
}

static gboolean ignorance_rule_valid(const gchar *ruletext, gint ruletype) {
	gboolean rv=FALSE;

#ifdef HAVE_REGEX_H
	regex_t reg;
#endif

	switch(ruletype) {
		case IGNORANCE_RULE_SIMPLETEXT:
		case IGNORANCE_RULE_REPEAT:
			rv=TRUE;
			break;
#ifdef HAVE_REGEX_H
		case IGNORANCE_RULE_REGEX:
			rv=!((gboolean)regcomp(&reg,ruletext,REG_EXTENDED | REG_NOSUB));
			rv=(rv && strlen(ruletext));
			regfree(&reg);
			break;
#endif
	}

	return rv;
}

static gboolean ignorance_rulename_valid(const gchar *rule_name) {
	return TRUE;
}

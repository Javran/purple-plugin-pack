/*--------------------------------------------------------------------------*
 * AUTOPROFILE                                                              *
 *                                                                          *
 * A Purple away message and profile manager that supports dynamic text       *
 *                                                                          *
 * AutoProfile is the legal property of its developers.  Please refer to    *
 * the COPYRIGHT file distributed with this source distribution.            *
 *                                                                          *
 * This program is free software; you can redistribute it and/or modify     *
 * it under the terms of the GNU General Public License as published by     *
 * the Free Software Foundation; either version 2 of the License, or        *
 * (at your option) any later version.                                      *
 *                                                                          *
 * This program is distributed in the hope that it will be useful,          *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 * GNU General Public License for more details.                             *
 *                                                                          *
 * You should have received a copy of the GNU General Public License        *
 * along with this program; if not, write to the Free Software              *
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA *
 *--------------------------------------------------------------------------*/

#include "../common/pp_internal.h"

#include "widget.h"
#include "utility.h"

#include <string.h>

static GStaticMutex widget_mutex = G_STATIC_MUTEX_INIT;
static GList *widgets = NULL;
static GHashTable *identifiers = NULL;
static GRand *r = NULL;

static char *widget_pref = "/plugins/gtk/autoprofile/widgets/widget_ids";

static void ap_widget_init_default_statuses ()
{
  // Make sure we don't keep on readding the default statuses if a user
  // deleted them
  if (!purple_prefs_exists (widget_pref)) {
    purple_prefs_add_none ("/plugins/gtk/autoprofile/widgets/42");
    purple_prefs_add_string (
        "/plugins/gtk/autoprofile/widgets/42/component", "Timestamp");
    purple_prefs_add_string (
        "/plugins/gtk/autoprofile/widgets/42/alias", "Timestamp");
    purple_prefs_add_string (
        "/plugins/gtk/autoprofile/widgets/42/timestamp_format",
        "Automatically created at %I:%M %p");
  }
}

void ap_widget_init () {
  GList *node;
        
  ap_widget_init_default_statuses ();

  node = g_list_append (NULL, g_strdup ("42"));
  purple_prefs_add_string_list (widget_pref, node);
  free_string_list (node);
}

/* Basic functions */
static gchar *strip_whitespace (const gchar *text) {
  gchar *result, *end, *search;

  while (isspace (*text)) {
    text++;
  }

  end = NULL;
  search = result = g_strdup (text);
  
  while (*search) {
    if (end == NULL && isspace (*search)) {
      end = search;
    }
    if (!isspace (*search)) {
      end = NULL;
    }
    search++;
  }

  if (end != NULL) *end = '\0';
  return result; 
}

static void update_widget_ids () {
  GList *cur_node, *ids;
  struct widget *cur_widget;

  ids = NULL;
  for (cur_node = widgets; cur_node != NULL; cur_node = cur_node->next) {
    cur_widget = (struct widget *) cur_node->data;
    ids = g_list_append (ids, cur_widget->wid);
  }

  purple_prefs_set_string_list (widget_pref, ids);
  g_list_free (ids);
}

// Mutex is ALREADY HELD when this function is called
static struct widget *ap_widget_find_internal (const gchar *search_text) {
  GList *cur_node;
  struct widget *cur_widget;
  gchar *alias;

  alias = strip_whitespace (search_text);

  cur_node = widgets;

  while (cur_node) {
    cur_widget = (struct widget *) cur_node->data;
    if (!purple_utf8_strcasecmp (alias, cur_widget->alias)) {
      free (alias);
      return cur_widget;
    }
    cur_node = cur_node->next;
  }

  free (alias);
  return NULL;
}

struct widget *ap_widget_find (const gchar *search_text) { 
  struct widget *w;

  g_static_mutex_lock (&widget_mutex);
  w = ap_widget_find_internal (search_text);
  g_static_mutex_unlock (&widget_mutex);
  return w;
}

struct widget *ap_widget_find_by_identifier (const gchar *search_text) {
  struct widget *w;

  g_static_mutex_lock (&widget_mutex);
  w = (struct widget *) g_hash_table_lookup (identifiers, search_text);
  g_static_mutex_unlock (&widget_mutex);
  return w;
}

void ap_widget_start () {
  GList *widget_identifiers, *widget_identifiers_start;
  GString *pref_name;
  const gchar *identifier, *component_identifier;
  struct component *comp;
  struct widget *w;

  g_static_mutex_lock (&widget_mutex);

  r = g_rand_new ();

  widgets = NULL;
  identifiers = g_hash_table_new (g_str_hash, g_str_equal);

  pref_name = g_string_new ("");
  widget_identifiers_start = purple_prefs_get_string_list (widget_pref);

  for (widget_identifiers = widget_identifiers_start;
       widget_identifiers != NULL;
       widget_identifiers = widget_identifiers->next) {
    g_string_printf (pref_name, 
      "/plugins/gtk/autoprofile/widgets/%s/component",
      (gchar *) widget_identifiers->data);

    component_identifier = purple_prefs_get_string (pref_name->str);
    if (component_identifier == NULL) {
      ap_debug_error ("widget", "widget does not have component information");
      continue;
    }

    comp = ap_component_get_component (component_identifier);

    if (comp == NULL) {
      ap_debug_error ("widget", "no component matches widget identifier");
      continue;
    } 

    g_string_printf (pref_name, 
      "/plugins/gtk/autoprofile/widgets/%s/alias",
      (gchar *) widget_identifiers->data);

    identifier = purple_prefs_get_string (pref_name->str);
    if (identifier == NULL) {
      ap_debug_error ("widget", "widget does not have alias information");
      continue;
    }
 
    w = ap_widget_find_internal (identifier); 
    if (w != NULL) {
      ap_debug_error ("widget", "widget alias already in use");
      continue;
    }

    w = (struct widget *) malloc (sizeof (struct widget));
    w->alias = g_strdup (identifier);
    w->wid = g_strdup ((gchar *) widget_identifiers->data);
    w->component = comp;
		w->data = g_hash_table_new (NULL, NULL);

    widgets = g_list_append (widgets, w);
    g_hash_table_insert (identifiers, w->wid, w);

    if (w->component->load) {
      w->component->load (w);
    }

    g_string_printf (pref_name, 
      "loaded saved widget with alias %s and identifier %s",
      w->alias,
      w->wid);
    ap_debug_misc ("widget", pref_name->str);
  }

  free_string_list (widget_identifiers_start);
  g_string_free (pref_name, TRUE);
  
  g_static_mutex_unlock (&widget_mutex);

  ap_widget_gtk_start (); 
}

void ap_widget_finish () { 
  GList *tmp;
  struct widget *w;

  g_static_mutex_lock (&widget_mutex);

  ap_widget_gtk_finish ();

  g_hash_table_destroy (identifiers);
  identifiers = NULL;

  while (widgets) {
    w = (struct widget *) widgets->data;

    if (w->component->unload) {
      w->component->unload (w);
    } 

	  g_hash_table_destroy (w->data);
    free (w->alias);
    free (w->wid);
    free (w);

    tmp = widgets->next;
    g_list_free_1 (widgets);
    widgets = tmp;
  }

  g_rand_free (r);
  r = NULL;

  g_static_mutex_unlock (&widget_mutex);
}

gboolean ap_widget_has_content_changed () { 
  GList *node;
  struct widget *w;
  gboolean changed = FALSE;
  
  g_static_mutex_lock (&widget_mutex);
  for (node = widgets; node != NULL; node = node->next) {  
    w = (struct widget *) node->data;
    if (w->component->has_content_changed == NULL ||
        w->component->has_content_changed (w)) {
      changed = TRUE;
      break;
    }
  }
  
  g_static_mutex_unlock (&widget_mutex);
  return changed; 
}

GList *ap_widget_get_widgets () { 
  GList *result;
  g_static_mutex_lock (&widget_mutex);
  result = g_list_copy (widgets);
  g_static_mutex_unlock (&widget_mutex);
  return result;
}

struct widget *ap_widget_create (struct component *comp)
{ 
  struct widget *w;
  GString *s;
  gchar *identifier, *alias;
  int i;
  GList *node;

  g_static_mutex_lock (&widget_mutex);

  // Sanity check to make sure we dont "delete" old widgets by
  // overriding old pref
  if (identifiers == NULL) {
    ap_debug_warn ("widget", 
      "tried to create widget when variables unitialized");
    g_static_mutex_unlock (&widget_mutex);
    return NULL;
  }

  ap_debug ("widget", "instantiating new widget from component");

  s = g_string_new ("");

  // Get alias
  w = ap_widget_find_internal (comp->identifier);
  alias = NULL; // Stupid compiler

  if (w == NULL) {
    alias = g_strdup (comp->identifier);
  } else {
    for (i = 1; i < 10000; i++) {
      g_string_printf (s, "%s%d", comp->identifier, i);
      w = ap_widget_find_internal (s->str);
      if (w == NULL) {
        alias = g_strdup (s->str);
        break;
      }
    }

    if (i == 10000) {
      // This would happen....very very rarely...
      ap_debug_error ("widget", "ran out of aliases for component");
      g_string_free (s, TRUE);
      g_static_mutex_unlock (&widget_mutex);
      return NULL;
    }
  }

  // Get identifier
  while (TRUE) {
    i = g_rand_int (r);
    g_string_printf (s, "%d", i);

    node = widgets;

    while (node) {
      w = (struct widget *) node->data;  
      if (!strcmp (s->str, w->wid)) {
        break;
      }
      node = node->next;
    }

    if (node == NULL) {
      identifier = g_strdup (s->str);
      break;
    }
  }

  w = (struct widget *) malloc (sizeof (struct widget));
  w->alias = alias;
  w->wid = identifier;
  w->component = comp;
	w->data = g_hash_table_new (NULL, NULL);

  widgets = g_list_append (widgets, w);
  g_hash_table_insert (identifiers, w->wid, w);

  // Modify Purple prefs
  update_widget_ids ();

  g_string_printf (s, "/plugins/gtk/autoprofile/widgets/%s", w->wid);
  purple_prefs_add_none (s->str);

  g_string_printf (s, "/plugins/gtk/autoprofile/widgets/%s/component", 
		w->wid);
  purple_prefs_add_string (s->str, w->component->identifier);

  g_string_printf (s, "/plugins/gtk/autoprofile/widgets/%s/alias", w->wid);
  purple_prefs_add_string (s->str, w->alias);

  // Initialize widget
  if (w->component->init_pref) {
    w->component->init_pref (w);
  }
  if (w->component->load) {
    w->component->load (w);
  }

  // Cleanup
  g_string_printf (s, "Created widget with alias %s and identifier %s",
    alias, identifier);
  ap_debug ("widget", s->str);

  g_string_free (s, TRUE);


  g_static_mutex_unlock (&widget_mutex);

  return w;
}

void ap_widget_delete (struct widget *w) { 
  GString *s;

  if (w == NULL) {
    ap_debug_error ("widget", "attempt to delete NULL widget");
    return;
  }

  g_static_mutex_lock (&widget_mutex);

  // Sanity check to make sure we dont "delete" old widgets by
  // overriding old pref
  if (identifiers == NULL) {
    ap_debug_warn ("widget", 
      "tried to delete widget when variables unitialized");
    g_static_mutex_unlock (&widget_mutex);
    return;
  }

  s = g_string_new ("");

  g_string_printf (s, "Deleting widget with alias %s and identifier %s",
    w->alias, w->wid);
  ap_debug ("widget", s->str);

  widgets = g_list_remove (widgets, w);
  g_hash_table_remove (identifiers, w->wid);

  update_widget_ids ();

  g_string_printf (s, "/plugins/gtk/autoprofile/widgets/%s", w->wid);
  purple_prefs_remove (s->str);

  g_string_free (s, TRUE);

  if (w->component->unload) {
    w->component->unload (w);
  }

	g_hash_table_destroy (w->data);
  free (w->wid);
  free (w->alias);
  free (w);

  g_static_mutex_unlock (&widget_mutex);
}

// TRUE if rename succeeds, FALSE otherwise
gboolean ap_widget_rename (struct widget *orig, const gchar *new_alias) { 
  struct widget *w;
  GString *s;
  gchar *orig_alias;

  g_static_mutex_lock (&widget_mutex);

  w = ap_widget_find_internal (new_alias);
  if (w != NULL && w != orig) {
    g_static_mutex_unlock (&widget_mutex);
    return FALSE;
  }

  orig_alias = orig->alias;
  orig->alias = g_strdup (new_alias);

  s = g_string_new ("");

  g_string_printf (s, "/plugins/gtk/autoprofile/widgets/%s/alias", orig->wid);
  purple_prefs_set_string (s->str, new_alias);

  g_string_printf (s, "Changed alias of widget from %s to %s", 
    orig_alias, new_alias);
  ap_debug ("widget", s->str);

  free (orig_alias);
  g_string_free (s, TRUE); 

  g_static_mutex_unlock (&widget_mutex);
  return TRUE;
}

/* Widget data galore! */
void ap_widget_set_data (struct widget *w, int id, gpointer data) {
  g_static_mutex_lock (&widget_mutex);
	g_hash_table_insert (w->data, GINT_TO_POINTER(id), data);
  g_static_mutex_unlock (&widget_mutex);
}

gpointer ap_widget_get_data (struct widget *w, int id) {
	gpointer result;

  g_static_mutex_lock (&widget_mutex);
	result = g_hash_table_lookup (w->data, GINT_TO_POINTER(id));
  g_static_mutex_unlock (&widget_mutex);

	return result;
}

/* Widget preferences galore! */
gchar *ap_prefs_get_pref_name (struct widget *w, const char *name) {
  GString *s;
  gchar *result;
  
  s = g_string_new ("");
  g_string_append (s, "/plugins/gtk/autoprofile/widgets/");
  g_string_append_printf (s, "%s/%s", w->wid, name); 

  result = s->str;
  g_string_free (s, FALSE);
  return result;
}

void ap_prefs_add_bool (struct widget *w, const char *name, gboolean value) {
  gchar *pref = ap_prefs_get_pref_name (w, name);
  purple_prefs_add_bool (pref, value);
  free (pref);
}

void ap_prefs_add_int (struct widget *w, const char *name, int value) {
  gchar *pref = ap_prefs_get_pref_name (w, name);
  purple_prefs_add_int (pref, value);
  free (pref);
}

void ap_prefs_add_none (struct widget *w, const char *name) {
  gchar *pref = ap_prefs_get_pref_name (w, name);
  purple_prefs_add_none (pref);
  free (pref);
}

void ap_prefs_add_string (struct widget *w, const char *name, 
  const char *value) 
{
  gchar *pref = ap_prefs_get_pref_name (w, name);
  purple_prefs_add_string (pref, value);
  free (pref);
}

void ap_prefs_add_string_list (struct widget *w, const char *name, 
  GList *value) 
{
  gchar *pref = ap_prefs_get_pref_name (w, name);
  purple_prefs_add_string_list (pref, value);
  free (pref);
}

gboolean ap_prefs_get_bool (struct widget *w, const char *name) {
  gboolean result;
  gchar *pref = ap_prefs_get_pref_name (w, name);
  result = purple_prefs_get_bool (pref);
  free (pref);
  return result;
}

int ap_prefs_get_int (struct widget *w, const char *name) {
  int result;
  gchar *pref = ap_prefs_get_pref_name (w, name);
  result = purple_prefs_get_int (pref);
  free (pref);
  return result;
}

const char *ap_prefs_get_string (struct widget *w, const char *name) {
  const char *result;
  gchar *pref = ap_prefs_get_pref_name (w, name);
  result = purple_prefs_get_string (pref);
  free (pref);
  return result;
}

GList *ap_prefs_get_string_list (struct widget *w, const char *name) {
  GList *result;
  gchar *pref = ap_prefs_get_pref_name (w, name);
  result = purple_prefs_get_string_list (pref);
  free (pref);
  return result;
}

void ap_prefs_set_bool (struct widget *w, const char *name, gboolean value) {
  gchar *pref = ap_prefs_get_pref_name (w, name);
  purple_prefs_set_bool (pref, value);
  free (pref);
  ap_widget_prefs_updated (w);
}

void ap_prefs_set_int (struct widget *w, const char *name, int value) {
  gchar *pref = ap_prefs_get_pref_name (w, name);
  purple_prefs_set_int (pref, value);
  free (pref);
  ap_widget_prefs_updated (w);
}

void ap_prefs_set_string (struct widget *w, const char *name, 
  const char *value) 
{
  gchar *pref = ap_prefs_get_pref_name (w, name);
  purple_prefs_set_string (pref, value);
  free (pref);
  ap_widget_prefs_updated (w);
}

void ap_prefs_set_string_list (struct widget *w, const char *name, 
  GList *value) 
{
  gchar *pref = ap_prefs_get_pref_name (w, name);
  purple_prefs_set_string_list (pref, value);
  free (pref);
  ap_widget_prefs_updated (w);
}


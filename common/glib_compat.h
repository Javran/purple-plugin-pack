/*
 * Purple Plugin Pack - GLib Compatibility Header
 *
 * This code borrowed from GLib (http://www.gtk.org/) for backward
 * compatibility with ancient versions of GLib.
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

#include <string.h>

#if !GLIB_VERSION_CHECK(2,2,0)
gboolean
g_str_has_suffix (const gchar  *str,
		  const gchar  *suffix)
{
  int str_len;
  int suffix_len;
  
  g_return_val_if_fail (str != NULL, FALSE);
  g_return_val_if_fail (suffix != NULL, FALSE);

  str_len = strlen (str);
  suffix_len = strlen (suffix);

  if (str_len < suffix_len)
    return FALSE;

  return strcmp (str + str_len - suffix_len, suffix) == 0;
}

gboolean
g_str_has_prefix (const gchar  *str,
		  const gchar  *prefix)
{
  int str_len;
  int prefix_len;
  
  g_return_val_if_fail (str != NULL, FALSE);
  g_return_val_if_fail (prefix != NULL, FALSE);

  str_len = strlen (str);
  prefix_len = strlen (prefix);

  if (str_len < prefix_len)
    return FALSE;
  
  return strncmp (str, prefix, prefix_len) == 0;
}
#endif

/* this check is backwards now because we need to do stuff both if the builder
 * has 2.4.0 and if the builder doesn't have 2.4.0 */
#if GLIB_CHECK_VERSION(2,4,0)
#  include <glib/gi18n-lib.h>
#else
#  include <locale.h>
#  include <libintl.h>
#  define _(String) dgettext (GETTEXT_PACKAGE, String)
#  define Q_(String) g_strip_context ((String), dgettext (GETTEXT_PACKAGE, String))
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
static gchar **
g_strsplit_set (const gchar *string,
		const gchar *delimiters,
		gint         max_tokens)
{
	gboolean delim_table[256];
	GSList *tokens, *list;
	gint n_tokens;
	const gchar *s;
	const gchar *current;
	gchar *token;
	gchar **result;

	g_return_val_if_fail (string != NULL, NULL);
	g_return_val_if_fail (delimiters != NULL, NULL);

	if (max_tokens < 1)
		max_tokens = G_MAXINT;

	if (*string == '\0')
	{
		result = g_new (char *, 1);
		result[0] = NULL;
		return result;
	}

	memset (delim_table, FALSE, sizeof (delim_table));
	for (s = delimiters; *s != '\0'; ++s)
		delim_table[*(guchar *)s] = TRUE;

	tokens = NULL;
	n_tokens = 0;

	s = current = string;
	while (*s != '\0')
	{
		if (delim_table[*(guchar *)s] && n_tokens + 1 < max_tokens)
		{
			gchar *token;

			token = g_strndup (current, s - current);
			tokens = g_slist_prepend (tokens, token);
			++n_tokens;

			current = s + 1;
		}

		++s;
	}

	token = g_strndup (current, s - current);
	tokens = g_slist_prepend (tokens, token);
	++n_tokens;

	result = g_new (gchar *, n_tokens + 1);

	result[n_tokens] = NULL;
	for (list = tokens; list != NULL; list = list->next)
		result[--n_tokens] = list->data;

	g_slist_free (tokens);

	return result;
}
#endif

#if !GLIB_VERSION_CHECK(2,6,0)
static guint
g_strv_length (gchar **str_array)
{
  guint i = 0;

  g_return_val_if_fail (str_array != NULL, 0);

  while (str_array[i])
    ++i;

  return i;
}
#endif

#include <string.h>

#ifndef g_str_has_suffix
/**
 * g_str_has_suffix:
 * @str: a nul-terminated string.
 * @suffix: the nul-terminated suffix to look for.
 *
 * Looks whether the string @str ends with @suffix.
 *
 * Return value: TRUE if @str end with @suffix, FALSE otherwise.
 **/
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
#endif

#ifndef g_str_has_prefix
/**
 * g_str_has_prefix:
 * @str: a nul-terminated string.
 * @prefix: the nul-terminated prefix to look for.
 *
 * Looks whether the string @str begins with @prefix.
 *
 * Return value: TRUE if @str begins with @prefix, FALSE otherwise.
 **/
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

#ifndef g_strsplit_set
/**
  * g_strsplit_set:
  * @string: The string to be tokenized
  * @delimiters: A nul-terminated string containing bytes that are used
  *              to split the string.
  * @max_tokens: The maximum number of tokens to split @string into. 
  *              If this is less than 1, the string is split completely
  * 
  * Splits @string into a number of tokens not containing any of the characters
  * in @delimiter. A token is the (possibly empty) longest string that does not
  * contain any of the characters in @delimiters. If @max_tokens is reached, the
  * remainder is appended to the last token.
  *
  * For example the result of g_strsplit_set ("abc:def/ghi", ":/", -1) is a
  * %NULL-terminated vector containing the three strings "abc", "def", 
  * and "ghi".
  *
  * The result if g_strsplit_set (":def/ghi:", ":/", -1) is a %NULL-terminated
  * vector containing the four strings "", "def", "ghi", and "".
  * 
  * As a special case, the result of splitting the empty string "" is an empty
  * vector, not a vector containing a single string. The reason for this
  * special case is that being able to represent a empty vector is typically
  * more useful than consistent handling of empty elements. If you do need
  * to represent empty elements, you'll need to check for the empty string
  * before calling g_strsplit_set().
  *
  * Note that this function works on bytes not characters, so it can't be used 
  * to delimit UTF-8 strings for anything but ASCII characters.
  * 
  * Return value: a newly-allocated %NULL-terminated array of strings. Use 
  *    g_strfreev() to free it.
  * 
  * Since: 2.4
  **/
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

#ifndef g_strv_length
/**
 * g_strv_length:
 * @str_array: a %NULL-terminated array of strings.
 * 
 * Returns the length of the given %NULL-terminated 
 * string array @str_array.
 * 
 * Return value: length of @str_array.
 *
 * From the file gstrfuncs.c
 *
 * Since: 2.6
 **/
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

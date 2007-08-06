/*
 * Purple Plugin Pack
 * Copyright (C) 2003-2005
 * See AUTHORS for a list of all authors
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
#ifndef PP_INTERNAL_H
#define PP_INTERNAL_H

#ifdef HAVE_CONFIG_H
#  include "../pp_config.h"
#endif

#include <glib.h>

/* This works around the lack of i18n support in old glib.  Needed because
 * we moved to using intltool and glib wrappings. */
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
#endif

#ifdef _WIN32
# include <win32dep.h>
#endif

/* This works around the lack of G_GNUC_NULL_TERMINATED in old glib and the
 * lack of the NULL sentinel in GCC older than 4.0.0 and non-GCC compilers */
#ifndef G_GNUC_NULL_TERMINATED
#  if     __GNUC__ >= 4
#    define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
#  else
#    define G_GNUC_NULL_TERMINATED
#  endif
#endif

/* Every plugin for libpurple, Pidgin, or Finch needs to define PURPLE_PLUGINS
 * in order to compile and run correctly. Define it here for simplicity */
#define PURPLE_PLUGINS

/* Every plugin for libpurple, Pidgin, or Finch needs to include version.h
 * to ensure it has the version macros, so include it here for simplicity */
#include <version.h>

#endif /* PP_INTERNAL_H */

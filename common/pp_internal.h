/*
 * Purple Plugin Pack
 * Copyright (C) 2003-2008
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301, USA.
 */
#ifndef PP_INTERNAL_H
#define PP_INTERNAL_H

#ifdef HAVE_CONFIG_H
#  include "../pp_config.h"
#endif

#include <glib.h>

#include "../common/glib_compat.h"

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

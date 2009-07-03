/*
 * Guifications - The end all, be all, toaster popup plugin
 * Copyright (C) 2003-2005 Gary Kramlich
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
#ifndef IG_INTERNAL_H
#define IG_INTERNAL_H

#include <glib.h>

#define IGNORANCE_PLUGIN_ID "gtk-bleeter-ignorance"
#define SHORTDESC "Ignorance filter"

#ifndef IGNORANCE_CONFDIR
# define IGNORANCE_CONFDIR purple_user_dir()
#endif

#define EXEC_TIMEOUT 10

#define GREATER(x,y) ((x)?((x)>(y)):(y))

#if ((PURPLE_MAJOR_VERSION) < 2)
#define PURPLE_CONV_TYPE_CHAT PURPLE_CONV_CHAT
#define PURPLE_CONV_TYPE_IM PURPLE_CONV_IM
#endif

#if GLIB_CHECK_VERSION(2,6,0)
# include <glib/gstdio.h>
#endif

#ifdef _WIN32
# include <win32dep.h>
#endif

#if !GLIB_CHECK_VERSION(2,6,0)
# define g_freopen freopen
# define g_fopen fopen
# define g_rmdir rmdir
# define g_remove remove
# define g_unlink unlink
# define g_lstat lstat
# define g_stat stat
# define g_mkdir mkdir
# define g_rename rename
# define g_open open
#endif

#ifdef HAVE_ENDIAN_H
# include <endian.h>
#endif

#define MSG_LEN 2048
/* The above should normally be the same as BUF_LEN,
 * but just so we're explicitly asking for the max message
 * length. */
#define BUF_LEN MSG_LEN
#define BUF_LONG BUF_LEN * 2

#endif

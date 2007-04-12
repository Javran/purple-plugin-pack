/*
 * Purple Plugin Pack
 * Copyright (C) 2003-2005
 * See ../AUTHORS for a list of all authors
 *
 * listhandler: Provides importing, exporting, and copying functions
 * 				for accounts' buddy lists.
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

#ifdef HAVE_CONFIG_H
#	include "../gpp_config.h"
#endif

#include <glib.h>
#include <string.h>

/* this has to be defined for the plugin to work properly */
#define PURPLE_PLUGINS

/* Purple headers */
#include <account.h>
#include <blist.h>
#include <debug.h>
#include <plugin.h>
#include <request.h>
#include <version.h>
#include <xmlnode.h>

#include "../common/i18n.h"

#include "lh_util.h"

/* we send this to the request API so it is probably a good idea to make sure
 * the other files that need it have it */
extern PurplePlugin *listhandler;


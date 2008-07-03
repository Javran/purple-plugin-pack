/*
 * irssi - Implements several irssi features for Purple
 * Copyright (C) 2005-2008 Gary Kramlich <grim@reaperworld.com>
 * Copyright (C) 2006-2008 John Bailey <rekkanoryo@rekkanoryo.org>
 * Copyright (C) 2006-2008 Sadrul Habib Chowdhury <sadrul@users.sourceforge.net>
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

#define PLUGIN_ID			"gtk-plugin_pack-irssi"
#define PLUGIN_STATIC_NAME	"irssi"
#define PLUGIN_AUTHOR		"\n" \
							"\tGary Kramlich <grim@reaperworld.com>\n" \
							"\tJohn Bailey <rekkanoryo@rekkanoryo.org>\n" \
							"\tSadrul Habib Chowdhury <sadrul@users.sourceforge.net>"

#define PREFS_ROOT_PARENT	"/pidgin/plugins"
#define PREFS_ROOT			"/pidgin/plugins/" PLUGIN_ID
#define TEXTFMT_PREF		PREFS_ROOT "/textfmt"
#define DATECHANGE_PREF		PREFS_ROOT "/datechange"
#define SENDNEWYEAR_PREF	PREFS_ROOT "/happynewyear"

#include "datechange.h"
#include "lastlog.h"
#include "layout.h"
#include "textfmt.h"
#include "window.h"


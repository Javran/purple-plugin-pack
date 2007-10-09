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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301, USA.
 */

void lh_util_add_buddy(const gchar *group, PurpleGroup *purple_group,
			const gchar *buddy, const gchar *alias, PurpleAccount *account,
			const gchar *buddynotes, gint signed_on, gint signed_off,
			gint lastseen, gint last_seen, const gchar *gf_theme,
			const gchar *icon_file, gchar *lastsaid);

void lh_util_add_to_blist(GList *buddies, GList *groups);


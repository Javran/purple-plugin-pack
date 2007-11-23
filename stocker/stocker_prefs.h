/*
 * Stocker - Adds a stock ticker to the buddy list
 * Copyright (C) 2005-2007 Gary Kramlich <grim@reaperworld.com>
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
#ifndef STOCKER_PREFS_H
#define STOCKER_PREFS_H

#include <gtk/gtk.h>

#include <plugin.h>

#define PREF_MY			"/plugins/gtk/amc_grim"
#define PREF_ROOT		"/plugins/gtk/amc_grim/stocker"
#define PREF_SYMBOLS	"/plugins/gtk/amc_grim/stocker/symbols"
#define PREF_INTERVAL	"/plugins/gtk/amc_grim/stocker/interval"

G_BEGIN_DECLS

void stocker_prefs_init(void);
GtkWidget *stocker_prefs_get_frame(PurplePlugin *plugin);

G_END_DECLS

#endif /* STOCKER_PREFS_H */

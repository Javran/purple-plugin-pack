/*
 * Buddy Time - Displays a buddy's local time
 *
 * A libpurple plugin that allows you to configure a timezone on a per-contact
 * basis so it can display the localtime of your contact when a conversation
 * starts. Convenient if you deal with contacts from many parts of the
 * world.
 *
 * Copyright (C) 2006-2007, Richard Laager <rlaager@users.sf.net>
 * Copyright (C) 2006, Martijn van Oosterhout <kleptog@svana.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
#ifndef _BUDDYTIME_H_
#define _BUDDYTIME_H_

#define CORE_PLUGIN_STATIC_NAME   "buddytime"
#define CORE_PLUGIN_ID            "core-kleptog-" CORE_PLUGIN_STATIC_NAME

#define PLUGIN_AUTHOR             "Martijn van Oosterhout <kleptog@svana.org>" \
                                  "\n\t\t\tRichard Laager <rlaager@pidgin.im>"

#define BUDDYTIME_BUDDY_GET_TIMEZONE "buddy_get_timezone"
#define BUDDYTIME_TIMEZONE_GET_TIME  "timezone_get_time"

typedef struct _BuddyTimeUiOps BuddyTimeUiOps;

struct _BuddyTimeUiOps
{
    void *(*create_menu)(const char *selected);                 /**< Creates a timezone menu. */
    const char * (*get_timezone_menu_selection)(void *ui_data); /**< Retrieves the menu setting. */

    void (*_buddytime_reserved1)(void);
    void (*_buddytime_reserved2)(void);
    void (*_buddytime_reserved3)(void);
    void (*_buddytime_reserved4)(void);
};

#endif /* _BUDDYTIME_H_ */

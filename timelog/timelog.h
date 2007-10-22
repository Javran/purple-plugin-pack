/*
 * TimeLog plugin.
 *
 * Copyright (c) 2006 Jon Oberheide.
 *
 * $Id: gaim-timelog.h,v 1.30 2005/12/24 05:14:55 binaryjono Exp $
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

#ifndef _TIMELOG_H_
#define _TIMELOG_H

#define TIMELOG_PLUGIN_ID	"gtk-binaryjono-timelog"
#define TIMELOG_TITLE		_("TimeLog")

#define tl_debug(fmt, ...)	purple_debug(PURPLE_DEBUG_INFO, TIMELOG_TITLE, \
							fmt, ## __VA_ARGS__);
#define tl_error(fmt, ...)	purple_debug(PURPLE_DEBUG_ERROR, TIMELOG_TITLE, \
							fmt, ## __VA_ARGS__);

#endif

/*
 * LastLog - Searches the conversation log for a word or phrase
 * Copyright (C) 2006 John Bailey <rekkanoryo@rekkanoryo.org>
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

#if HAVE_CONFIG_H
# include "../gpp_config.h"
#endif /* HAVE_CONFIG_H */

static GaimPluginInfo lastlog_info =
{
	GAIM_PLUGIN_MAGIC,
	GAIM_MAJOR_VERSION,                             /**< major version  */
	GAIM_MINOR_VERSION,
	GAIM_PLUGIN_STANDARD,                           /**< type           */
	NULL,                                           /**< ui_requirement */
	0,                                              /**< flags          */
	NULL,                                           /**< dependencies   */
	GAIM_PRIORITY_DEFAULT,                          /**< priority       */
	"core-plugin_pack-lastlog",                     /**< id             */
	NULL,                                           /**< name           */
	VERSION,                                        /**< version        */
	NULL,                                           /**  summary        */
	NULL,                                           /**  description    */
	"John Bailey<rekkanoryo@rekkanoryo.org",        /**< author         */
	GPP_WEBSITE,                                    /**< homepage       */
	plugin_load,                                    /**< load           */
	NULL,                                           /**< unload         */
	NULL,                                           /**< destroy        */
	NULL,                                           /**< ui_info        */
    NULL,                                           /**< extra_info     */
	NULL,                                           /**< prefs_info     */
	NULL                                            /**< actions        */
};

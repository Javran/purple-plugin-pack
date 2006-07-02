/*
 * Chronic - Remote sound play triggering
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

static GaimPluginInfo chronic_info =
{
	GAIM_PLUGIN_MAGIC,		/* magic?  do you think i'm gullible enough to
							 * believe in magic? */
	GAIM_MAJOR_VERSION,		/* bet you can't guess what this is */
	GAIM_MINOR_VERSION,		/* this either */
	GAIM_PLUGIN_STANDARD,	/* and what about this? */
	NULL,
	NULL,
	GAIM_PRIORITY_DEFAULT,
	"core-plugin_pack-chronic",
	NULL,
	GPP_VERSION,
	NULL,
	NULL,
	"John Bailey <rekkanoryo@rekkanoryo.org>",
	GPP_WEBSITE,
	/* comments below are temporary until i decide if i need those functions */
	NULL, /*chronic_load,*/
	NULL, /*chronic_unload,*/
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static void
chronic_init(GaimPlugin *plugin)
{
#ifdef ENABLE_NLS
	bindtextdomain(GPP_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GPP_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	chronic_info.name = _("Chronic");
	chronic_info.summary = _("Sound playing triggers");
	chronic_info.description = _("Allows buddies to remotely trigger sound"
			" playing in your instance of Gaim with {S &lt;sound&gt; or !sound"
			" &lt;sound&gt;.  Inspired by #guifications channel resident"
			" EvilDennisR and ancient versions of AOL.");
}

GAIM_INIT_PLUGIN(chronic, chronic_load, chronic_info)

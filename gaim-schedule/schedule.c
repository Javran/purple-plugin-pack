/*
 * Gaim-Schedule - Schedule reminders/pounces at specified times.
 * Copyright (C) 2006
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

#include "schedule.h"
#include <notify.h>
#include <xmlnode.h>
#include <util.h>

#include <stdarg.h>
#include <stdlib.h>

static GList *schedules;
static int timeout;

static void save_cb();
static void parse_schedule(xmlnode *node);
static void parse_when(GaimSchedule *schedule, xmlnode *when);
static void parse_action(GaimSchedule *schedule, xmlnode *action);
static void gaim_schedules_load();

#define TIMEOUT	60		/* update every 60 seconds */

#define MINUTE	60
#define HOUR	(MINUTE * 60)
#define	DAY		(HOUR * 24)
#define	YEAR	(DAY * 365)
#define	WEEK	(DAY * 7)

/*static int months[] = {-1, */

/* I think this is going to be rather ugly */

static int
days_in_month(int month, int year)
{
	int days[] = {31, -1, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	if (month == 1)
	{
		if (year %400 == 0)
			return 29;
		if (year % 100 == 0)
			return 28;
		if (year % 4 == 0)
			return 29;
		return 28;
	}
	else
		return days[month];
}

static void
calculate_timestamp_for_schedule(GaimSchedule *schedule)
{
	time_t now, then;
	struct tm tm, *nt;
	gboolean imin = FALSE, ihour = FALSE, iday = FALSE, imon = FALSE, iyear = FALSE;

	now = time(NULL);

	nt = localtime(&now);

	tm.tm_sec = 0;
	if ((tm.tm_min = schedule->minute) == -1)
	{
		tm.tm_min = nt->tm_min;
		imin = TRUE;
	}
	if ((tm.tm_hour = schedule->hour) == -1)
	{
		tm.tm_hour = nt->tm_hour;
		ihour = TRUE;
	}

	if ((tm.tm_mon = schedule->month) == -1)
	{
		tm.tm_mon = nt->tm_mon;
		imon = TRUE;
	}

	if ((tm.tm_year = schedule->year) == -1)
	{
		tm.tm_year = nt->tm_year;
		iyear = TRUE;
	}

	if (schedule->type == GAIM_SCHEDULE_TYPE_DATE)
	{
		if ((tm.tm_mday = schedule->d.date) == 0)
		{
			tm.tm_mday = nt->tm_mday;
			iday = TRUE;
		}
	}
	else
	{
		int wday = nt->tm_wday;
		int swday = schedule->d.day - 1;

		if (swday == 0)
		{
			iday = TRUE;
			tm.tm_mday = nt->tm_mday;
		}
		else
		{
			tm.tm_mday = nt->tm_mday + (7 + wday - swday) % 7;
		}
	}

	then = mktime(&tm);
	schedule->timestamp = then;

	if (tm.tm_year < nt->tm_year)
	{
		if (!iyear)
			return;
		tm.tm_year = nt->tm_year;
	}
	else if (tm.tm_year > nt->tm_year)
	{
		if (imon)
			tm.tm_mon = 0;
		if (iday)
			tm.tm_mday = 1;
		schedule->timestamp = mktime(&tm);
		return;
	}

	if (tm.tm_mon < nt->tm_mon)
	{
		if (!imon)
		{
			if (iyear)
				tm.tm_year++;
			if (iday)
				tm.tm_mday = 1;
			schedule->timestamp = mktime(&tm);
			return;
		}
		tm.tm_mon = nt->tm_mon;
	}
	else if (tm.tm_mon > nt->tm_mon)
	{
		if (iday)
			tm.tm_mday = 1;
		schedule->timestamp = mktime(&tm);
		return;
	}

	if (tm.tm_mday < nt->tm_mday)
	{
		if (!iday)
		{
			if (imon && tm.tm_mon < 11)
				tm.tm_mon++;
			else if (iyear)
			{
				tm.tm_year++;
				if (imon)
					tm.tm_mon = 0;
			}
			schedule->timestamp = mktime(&tm);
			return;
		}

		if (schedule->type == GAIM_SCHEDULE_TYPE_DATE)
		{
			tm.tm_mday = nt->tm_mday;
		}
		else
		{
			/* XXX: something about the weekday */
		}
	}

	if (tm.tm_mday > nt->tm_mday)
	{
		schedule->timestamp = mktime(&tm);
		return;
	}

	if (tm.tm_hour < nt->tm_hour)
	{
		if (!ihour)
		{
			if (iday && tm.tm_mday + 1 < days_in_month(tm.tm_mon, tm.tm_year))
				tm.tm_mday++;
			else if (imon && tm.tm_mon < 11)
			{
				tm.tm_mon++;
				if (iday)
					tm.tm_mday = 1;
			}
			else if (iyear)
			{
				tm.tm_year++;
				if (imon)
					tm.tm_mon = 0;
				if (iday)
					tm.tm_mday = 1;
			}

			schedule->timestamp = mktime(&tm);
			return;
		}
		tm.tm_hour = nt->tm_hour;
	}
	else if (tm.tm_hour > nt->tm_hour)
	{
		schedule->timestamp = mktime(&tm);
		return;
	}

	if (tm.tm_min < nt->tm_min)
	{
		if (!imin)
		{
			if (ihour && tm.tm_hour < 22)
				tm.tm_hour++;
			else if (iday && tm.tm_mday + 1 < days_in_month(tm.tm_mon, tm.tm_year))
			{
				tm.tm_mday++;
				if (ihour)
					tm.tm_hour = 0;
			}
			else if (imon && tm.tm_mon < 11)
			{
				tm.tm_mon++;
				if (iday)
					tm.tm_mday = 1;
				if (ihour)
					tm.tm_hour = 0;
			}
			else if (iyear)
			{
				tm.tm_year++;
				if (imon)
					tm.tm_mon = 0;
				if (iday)
					tm.tm_mday = 1;
				if (ihour)
					tm.tm_hour = 0;
			}
			schedule->timestamp = mktime(&tm);
			return;
		}
		tm.tm_min = nt->tm_min + 1;
	}
	schedule->timestamp = mktime(&tm);

	if (schedule->timestamp < time(NULL))
	{
		if (imin && tm.tm_min < 58)
			tm.tm_min++;
		else if (ihour && tm.tm_hour < 22)
		{
			tm.tm_hour++;
			if (imin)
				tm.tm_min = 0;
		}
		else if (iday && tm.tm_mday + 1 < days_in_month(tm.tm_mon, tm.tm_year))
		{
			tm.tm_mday++;
			if (ihour)
				tm.tm_hour = 0;
			if (imin)
				tm.tm_min = 0;
		}
		else if (imon && tm.tm_mon < 11)
		{
			tm.tm_mon++;
			if (iday)
				tm.tm_mday = 1;
			if (ihour)
				tm.tm_hour = 0;
			if (imin)
				tm.tm_min = 0;
		}
		else if (iyear)
		{
			tm.tm_year++;
			if (imon)
				tm.tm_mon = 0;
			if (iday)
				tm.tm_mday = 1;
			if (ihour)
				tm.tm_hour = 0;
			if (imin)
				tm.tm_min = 0;
		}
		schedule->timestamp = mktime(&tm);
	}
	
	return;
}

void
gaim_schedule_reschedule(GaimSchedule *schedule)
{
	calculate_timestamp_for_schedule(schedule);
	if (schedule->timestamp < time(NULL))
	{
		schedule->timestamp = 0;
		gaim_debug_warning("gaim-schedule", "schedule \"%s\" will not be executed (%s)\n", schedule->name,
					gaim_date_format_full(localtime(&schedule->timestamp)));
	}
	else
	{
		gaim_debug_info("gaim-schedule", "schedule \"%s\" will be executed at: %s\n", schedule->name,
					gaim_date_format_full(localtime(&schedule->timestamp)));
	}
}

static int
sort_schedules(gconstpointer a, gconstpointer b)
{
	const GaimSchedule *sa = a, *sb = b;

	if (sa->timestamp > sb->timestamp)
		return -1;
	else if (sa->timestamp == sb->timestamp)
		return 0;
	else
		return 1;
}

static void
recalculate_next_slots()
{
	GList *iter;

	for (iter = schedules; iter; iter = iter->next)
	{
		gaim_schedule_reschedule(iter->data);
	}

	schedules = g_list_sort(schedules, sort_schedules);
}

GaimSchedule *gaim_schedule_new()
{
	GaimSchedule *sch = g_new0(GaimSchedule, 1);

	return sch;
}

void gaim_schedule_add_action(GaimSchedule *schedule, ScheduleActionType type, ...)
{
	ScheduleAction *action;
	va_list vargs;

	action = g_new0(ScheduleAction, 1);
	action->type = type;

	va_start(vargs, type);
	switch (type)
	{
		case SCHEDULE_ACTION_POPUP:
			action->d.popup_message = g_strdup(va_arg(vargs, char *));
			break;
		case SCHEDULE_ACTION_CONV:
			action->d.send.message = g_strdup(va_arg(vargs, char*));
			action->d.send.who = g_strdup(va_arg(vargs, char *));
			action->d.send.account = va_arg(vargs, GaimAccount*);
			break;
		case SCHEDULE_ACTION_STATUS:
			action->d.status_title = g_strdup(va_arg(vargs, char *));
			break;
		default:
			g_free(action);
			g_return_if_reached();
	}
	va_end(vargs);
	schedule->actions = g_list_append(schedule->actions, action);
	save_cb();
}

void gaim_schedule_remove_action(GaimSchedule *schedule, ScheduleActionType type)
{
	GList *iter;

	for (iter = schedule->actions; iter; iter = iter->next)
	{
		ScheduleAction *action = iter->data;
		if (action->type == type)
		{
			gaim_schedule_action_destroy(action);
			schedule->actions = g_list_delete_link(schedule->actions, iter);
			break;
		}
	}
}

void gaim_schedule_action_activate(ScheduleAction *action)
{
	GaimConversation *conv;

	switch (action->type)
	{
		case SCHEDULE_ACTION_POPUP:
			gaim_notify_message(action, GAIM_NOTIFY_MSG_INFO, _("Schedule"), action->d.popup_message,
							NULL, NULL, NULL);
			break;
		case SCHEDULE_ACTION_CONV:
			conv = gaim_conversation_new(GAIM_CONV_TYPE_IM, action->d.send.account, action->d.send.who);
			gaim_conv_im_send_with_flags(GAIM_CONV_IM(conv), action->d.send.message, 0);
			break;
		default:
			gaim_debug_warning("gaim-schedule", "unimplemented action\n");
			break;
	}
}

void gaim_schedule_activate_actions(GaimSchedule *sch)
{
	GList *iter;
	
	for (iter = sch->actions; iter; iter = iter->next)
	{
		gaim_schedule_action_activate(iter->data);
	}
}

void gaim_schedule_action_destroy(ScheduleAction *action)
{
	switch (action->type)
	{
		case SCHEDULE_ACTION_POPUP:
			g_free(action->d.popup_message);
			break;
		case SCHEDULE_ACTION_CONV:
			g_free(action->d.send.message);
			g_free(action->d.send.who);
			break;
		case SCHEDULE_ACTION_STATUS:
			g_free(action->d.status_title);
			break;
		default:
			gaim_debug_warning("gaim-schedule", "unknown action type\n");
			break;
	}
	g_free(action);
	gaim_notify_close_with_handle(action);
}

void gaim_schedule_destroy(GaimSchedule *schedule)
{
	while (schedule->actions)
	{
		ScheduleAction *action = schedule->actions->data;
		gaim_schedule_action_destroy(action);
		schedule->actions = g_list_delete_link(schedule->actions, schedule->actions);
	}
	g_free(schedule);

	schedules = g_list_remove(schedules, schedule);
}

GList *gaim_schedules_get_all()
{
	return schedules;
}


void gaim_schedules_add(GaimSchedule *schedule)
{
	schedules = g_list_append(schedules, schedule);
}

static gboolean
check_and_execute(gpointer null)
{
	GaimSchedule *schedule;
	GList *iter = schedules;
	gboolean dirty = FALSE;
   
	if (iter == NULL)
		return TRUE;
	schedule = iter->data;
	while (schedule->timestamp && schedule->timestamp < time(NULL))
	{
		gaim_schedule_activate_actions(schedule);
		gaim_schedule_reschedule(schedule);
		dirty = TRUE;
		iter = iter->next;
		if (iter == NULL)
			break;
		schedule = iter->data;
	}
	if (dirty)
		schedules = g_list_sort(schedules, sort_schedules);
	return TRUE;
}

void gaim_schedule_init()
{
	gaim_schedules_load();
	recalculate_next_slots();
	timeout = g_timeout_add(10000, (GSourceFunc)check_and_execute, NULL);
}

void gaim_schedule_uninit()
{
	g_source_remove(timeout);
	save_cb();
	while (schedules)
	{
		gaim_schedule_destroy(schedules->data);
	}
}

void gaim_schedules_sync()
{
	save_cb();
}

/**
 * Read from XML
 */
static void
gaim_schedules_load()
{
	xmlnode *gaim, *schedule;

	gaim = gaim_util_read_xml_from_file("schedules.xml", _("list of schedules"));
	if (gaim == NULL)
		return;

	schedule = xmlnode_get_child(gaim, "schedules");
	if (schedule)
	{
		xmlnode *sch;
		for (sch = xmlnode_get_child(schedule, "schedule"); sch;
					sch = xmlnode_get_next_twin(sch))
		{
			parse_schedule(sch);
		}
	}

	xmlnode_free(gaim);
}

static void
parse_schedule(xmlnode *node)
{
	GaimSchedule *schedule;
	xmlnode *child;
	const char *name;

	child = xmlnode_get_child(node, "when");
	name = xmlnode_get_attrib(node, "name");

	if (name == NULL || child == NULL)
	{
		return;
	}

	schedule = gaim_schedule_new();
	schedule->name = g_strdup(name);

	schedules = g_list_append(schedules, schedule);

	parse_when(schedule, child);

	for (child = xmlnode_get_child(node, "action"); child; child = xmlnode_get_next_twin(child))
	{
		parse_action(schedule, child);
	}
}

static void
parse_when(GaimSchedule *schedule, xmlnode *when)
{
	int type = atoi(xmlnode_get_attrib(when, "type"));

	schedule->type = type;
	if (type == GAIM_SCHEDULE_TYPE_DATE)
		schedule->d.date = atoi(xmlnode_get_attrib(when, "date"));
	else
		schedule->d.day = atoi(xmlnode_get_attrib(when, "day"));

	schedule->month = atoi(xmlnode_get_attrib(when, "month"));
	schedule->year = atoi(xmlnode_get_attrib(when, "year"));
	schedule->hour = atoi(xmlnode_get_attrib(when, "hour"));
	schedule->minute = atoi(xmlnode_get_attrib(when, "minute"));
}

static void
parse_action(GaimSchedule *schedule, xmlnode *action)
{
	int type = atoi(xmlnode_get_attrib(action, "type"));
	xmlnode *data = xmlnode_get_child(action, "data"), *account, *message;

	switch (type)
	{
		case SCHEDULE_ACTION_POPUP:
			gaim_schedule_add_action(schedule, type, xmlnode_get_data(data));
			break;
		case SCHEDULE_ACTION_CONV:
			account = xmlnode_get_child(data, "account");
			message = xmlnode_get_child(data, "message");
			gaim_schedule_add_action(schedule, type, xmlnode_get_data(message),
						xmlnode_get_attrib(account, "who"),
						gaim_accounts_find(xmlnode_get_attrib(account, "name"),
									xmlnode_get_attrib(account, "prpl")));
			break;
		case SCHEDULE_ACTION_STATUS:
			gaim_schedule_add_action(schedule, type, xmlnode_get_data(action));
			break;
		default:
			g_return_if_reached();
	}
}

/**
 * Write into XML
 */

static void
xmlnode_set_attrib_int(xmlnode *node, const char *name, int value)
{
	char *v = g_strdup_printf("%d", value);
	xmlnode_set_attrib(node, name, v);
	g_free(v);
}

static xmlnode*
action_to_xmlnode(ScheduleAction *action)
{
	xmlnode *node, *child, *gchild;

	node = xmlnode_new("action");
	xmlnode_set_attrib_int(node, "type", action->type);

	child = xmlnode_new_child(node, "data");

	switch (action->type)
	{
		case SCHEDULE_ACTION_POPUP:
			/* XXX: need to do funky string-operations? */
			xmlnode_insert_data(child, action->d.popup_message, -1);
			break;
		case SCHEDULE_ACTION_CONV:
			gchild = xmlnode_new_child(child, "account");
			xmlnode_set_attrib(gchild, "prpl", gaim_account_get_protocol_id(action->d.send.account));
			xmlnode_set_attrib(gchild, "name", gaim_account_get_username(action->d.send.account));
			xmlnode_set_attrib(gchild, "who", action->d.send.who);

			gchild = xmlnode_new_child(child, "message");
			xmlnode_insert_data(gchild, action->d.send.message, -1);
			break;
		default:
			gaim_debug_warning("gaim-schedule", "unknown action type\n");
			break;
	}

	return node;
}

static xmlnode*
when_to_xmlnode(GaimSchedule *schedule)
{
	xmlnode *node;

	node = xmlnode_new("when");

	xmlnode_set_attrib_int(node, "type", schedule->type);

	switch (schedule->type)
	{
		case GAIM_SCHEDULE_TYPE_DATE:
			xmlnode_set_attrib_int(node, "date", schedule->d.date);
			break;
		case GAIM_SCHEDULE_TYPE_DAY:
			xmlnode_set_attrib_int(node, "day", schedule->d.day);
			break;
	}

	xmlnode_set_attrib_int(node, "month", schedule->month);

	xmlnode_set_attrib_int(node, "year", schedule->year);

	xmlnode_set_attrib_int(node, "hour", schedule->hour);

	xmlnode_set_attrib_int(node, "minute", schedule->minute);

	return node;
}

static xmlnode*
schedule_to_xmlnode(GaimSchedule *schedule)
{
	xmlnode *node, *child;
	GList *iter;

	node = xmlnode_new("schedule");
	xmlnode_set_attrib(node, "name", schedule->name);

	child = when_to_xmlnode(schedule);
	xmlnode_insert_child(node, child);
	
	for (iter = schedule->actions; iter; iter = iter->next)
	{
		xmlnode_insert_child(node, action_to_xmlnode(iter->data));
	}

	return node;
}

static xmlnode*
schedules_to_xmlnode()
{
	GList *iter;
	xmlnode *node, *child;

	node = xmlnode_new("gaim-schedule");
	xmlnode_set_attrib(node, "version", GPP_VERSION);

	child = xmlnode_new_child(node, "schedules");

	for (iter = schedules; iter; iter = iter->next)
	{
		GaimSchedule *schedule = iter->data;

		xmlnode_insert_child(child, schedule_to_xmlnode(schedule));
	}

	return node;
}

static void
save_cb()
{
	xmlnode *node;
	char *data;

	node = schedules_to_xmlnode();
	data = xmlnode_to_formatted_str(node, NULL);
	gaim_util_write_data_to_file("schedules.xml", data, -1);
	g_free(data);
	xmlnode_free(node);
}


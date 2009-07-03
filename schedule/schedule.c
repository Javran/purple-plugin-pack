/*
 * Purple-Schedule - Schedule reminders/pounces at specified times.
 * Copyright (C) 2006-2008
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

#include "schedule.h"

#include <notify.h>
#include <xmlnode.h>
#include <util.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static GList *schedules;
static int timeout;

static void save_cb();
static void parse_schedule(xmlnode *node);
static void parse_when(PurpleSchedule *schedule, xmlnode *when);
static void parse_action(PurpleSchedule *schedule, xmlnode *action);
static int sort_schedules(gconstpointer a, gconstpointer b);
static void purple_schedules_load();

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

static time_t
get_next(PurpleSchedule *s)
{
	struct
	{
		int mins[61];
		int hrs[25];
		int dates[32];
		int months[13];
		int years[3];
	} p;
	int i;
	int y, mo, d, h, mi;
	struct tm *tm, test;
	time_t tme;
	int first = 0;

	memset(&p, -1, sizeof(p));
	time(&tme);
	tm = localtime(&tme);

#define DECIDE(prop, st, count, offset) \
	do { \
		if (prop == -1) {  \
			first = 1; \
			for (i = 0; i < count; i++)  { \
				st[i] = ((first ? 0 : offset) + i) % count;  \
			} \
		} else  \
			st[0] = prop; \
	} while (0)

	DECIDE(s->minute, p.mins, 60, tm->tm_min); /* Minute */

	DECIDE(s->hour, p.hrs, 24, tm->tm_hour); /* Hour */

	DECIDE(s->d.date, p.dates, 31, tm->tm_mday); /* Date */

	DECIDE(s->month, p.months, 12, tm->tm_mon); /* Month */

	if (s->year == -1) {
		p.years[0] = tm->tm_year;
		p.years[1] = p.years[0] + 1;
	} else
		p.years[0] = s->year;

	test = *tm;

	for (y = 0; p.years[y] != -1; y++) {
		test.tm_year = p.years[y];
		for (mo = 0; p.months[mo] != -1; mo++) {
			test.tm_mon = p.months[mo];
			for (d = 0; p.dates[d] != -1; d++) {
				test.tm_mday = p.dates[d] + 1;     /* XXX: +1 is necessary */
				if (test.tm_mday > days_in_month(test.tm_mon, test.tm_year + 1900))
					continue;
				for (h = 0; p.hrs[h] != -1; h++) {
					test.tm_hour = p.hrs[h];
					for (mi = 0; p.mins[mi] != -1; mi++) {
						time_t then;

						test.tm_min = p.mins[mi];

						then = mktime(&test);
						if (then > tme)
							return then;
					}
				}
			}
		}
	}
	return -1;
}

static void
calculate_timestamp_for_schedule(PurpleSchedule *schedule)
{
	schedule->timestamp = get_next(schedule);
}

void
purple_schedule_reschedule(PurpleSchedule *schedule)
{
	calculate_timestamp_for_schedule(schedule);
	if (schedule->timestamp < time(NULL))
	{
		purple_debug_warning("purple-schedule", "schedule \"%s\" will not be executed (%s)\n", schedule->name,
					purple_date_format_full(localtime(&schedule->timestamp)));
		schedule->timestamp = 0;
	}
	else
	{
		purple_debug_info("purple-schedule", "schedule \"%s\" will be executed at: %s\n", schedule->name,
					purple_date_format_full(localtime(&schedule->timestamp)));
	}
}

static int
sort_schedules(gconstpointer a, gconstpointer b)
{
	const PurpleSchedule *sa = a, *sb = b;

	if (sa->timestamp < sb->timestamp)
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
		purple_schedule_reschedule(iter->data);
	}

	schedules = g_list_sort(schedules, sort_schedules);
}

PurpleSchedule *purple_schedule_new()
{
	PurpleSchedule *sch = g_new0(PurpleSchedule, 1);

	return sch;
}

void purple_schedule_add_action(PurpleSchedule *schedule, ScheduleActionType type, ...)
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
			action->d.send.account = va_arg(vargs, PurpleAccount*);
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

void purple_schedule_remove_action(PurpleSchedule *schedule, ScheduleActionType type)
{
	GList *iter;

	for (iter = schedule->actions; iter; iter = iter->next)
	{
		ScheduleAction *action = iter->data;
		if (action->type == type)
		{
			purple_schedule_action_destroy(action);
			schedule->actions = g_list_delete_link(schedule->actions, iter);
			break;
		}
	}
}

void purple_schedule_action_activate(ScheduleAction *action)
{
	PurpleConversation *conv;

	switch (action->type)
	{
		case SCHEDULE_ACTION_POPUP:
			purple_notify_message(action, PURPLE_NOTIFY_MSG_INFO, _("Schedule"), action->d.popup_message,
							NULL, NULL, NULL);
			break;
		case SCHEDULE_ACTION_CONV:
			conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, action->d.send.account, action->d.send.who);
			purple_conv_im_send_with_flags(PURPLE_CONV_IM(conv), action->d.send.message, 0);
			break;
		default:
			purple_debug_warning("purple-schedule", "unimplemented action\n");
			break;
	}
}

void purple_schedule_activate_actions(PurpleSchedule *sch)
{
	GList *iter;

	for (iter = sch->actions; iter; iter = iter->next)
	{
		purple_schedule_action_activate(iter->data);
	}
}

void purple_schedule_action_destroy(ScheduleAction *action)
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
			purple_debug_warning("purple-schedule", "unknown action type\n");
			break;
	}
	g_free(action);
	purple_notify_close_with_handle(action);
}

void purple_schedule_destroy(PurpleSchedule *schedule)
{
	while (schedule->actions)
	{
		ScheduleAction *action = schedule->actions->data;
		purple_schedule_action_destroy(action);
		schedule->actions = g_list_delete_link(schedule->actions, schedule->actions);
	}
	g_free(schedule);

	schedules = g_list_remove(schedules, schedule);
}

GList *purple_schedules_get_all()
{
	return schedules;
}


void purple_schedules_add(PurpleSchedule *schedule)
{
	schedules = g_list_append(schedules, schedule);
}

static gboolean
check_and_execute(gpointer null)
{
	PurpleSchedule *schedule;
	GList *iter = schedules;
	gboolean dirty = FALSE;

	if (iter == NULL)
		return TRUE;
	schedule = iter->data;
	while (schedule->timestamp && schedule->timestamp < time(NULL))
	{
		purple_schedule_activate_actions(schedule);
		purple_schedule_reschedule(schedule);
		iter = iter->next;
		dirty = TRUE;
		if (iter == NULL)
			break;
		schedule = iter->data;
	}
	schedules = g_list_sort(schedules, sort_schedules);
	return TRUE;
}

void purple_schedule_init()
{
	purple_schedules_load();
	recalculate_next_slots();
	timeout = g_timeout_add(10000, (GSourceFunc)check_and_execute, NULL);
}

void purple_schedule_uninit()
{
	g_source_remove(timeout);
	save_cb();
	while (schedules)
	{
		purple_schedule_destroy(schedules->data);
	}
}

void purple_schedules_sync()
{
	save_cb();
}

/**
 * Read from XML
 */
static void
purple_schedules_load()
{
	xmlnode *purple, *schedule;

	purple = purple_util_read_xml_from_file("schedules.xml", _("list of schedules"));
	if (purple == NULL)
		return;

	schedule = xmlnode_get_child(purple, "schedules");
	if (schedule)
	{
		xmlnode *sch;
		for (sch = xmlnode_get_child(schedule, "schedule"); sch;
					sch = xmlnode_get_next_twin(sch))
		{
			parse_schedule(sch);
		}
	}

	xmlnode_free(purple);
}

static void
parse_schedule(xmlnode *node)
{
	PurpleSchedule *schedule;
	xmlnode *child;
	const char *name;

	child = xmlnode_get_child(node, "when");
	name = xmlnode_get_attrib(node, "name");

	if (name == NULL || child == NULL)
	{
		return;
	}

	schedule = purple_schedule_new();
	schedule->name = g_strdup(name);

	schedules = g_list_append(schedules, schedule);

	parse_when(schedule, child);

	for (child = xmlnode_get_child(node, "action"); child; child = xmlnode_get_next_twin(child))
	{
		parse_action(schedule, child);
	}
}

static void
parse_when(PurpleSchedule *schedule, xmlnode *when)
{
	int type = atoi(xmlnode_get_attrib(when, "type"));

	schedule->type = type;
	if (type == PURPLE_SCHEDULE_TYPE_DATE)
		schedule->d.date = atoi(xmlnode_get_attrib(when, "date"));
	else
		schedule->d.day = atoi(xmlnode_get_attrib(when, "day"));

	schedule->month = atoi(xmlnode_get_attrib(when, "month"));
	schedule->year = atoi(xmlnode_get_attrib(when, "year"));
	schedule->hour = atoi(xmlnode_get_attrib(when, "hour"));
	schedule->minute = atoi(xmlnode_get_attrib(when, "minute"));
}

static void
parse_action(PurpleSchedule *schedule, xmlnode *action)
{
	int type = atoi(xmlnode_get_attrib(action, "type"));
	xmlnode *data = xmlnode_get_child(action, "data"), *account, *message;
	char *tmp;

	switch (type)
	{
		case SCHEDULE_ACTION_POPUP:
			tmp = xmlnode_get_data(data);
			purple_schedule_add_action(schedule, type, tmp);
			g_free(tmp);
			break;
		case SCHEDULE_ACTION_CONV:
			account = xmlnode_get_child(data, "account");
			message = xmlnode_get_child(data, "message");
			tmp = xmlnode_get_data(message);
			purple_schedule_add_action(schedule, type, tmp,
						xmlnode_get_attrib(account, "who"),
						purple_accounts_find(xmlnode_get_attrib(account, "name"),
									xmlnode_get_attrib(account, "prpl")));
			g_free(tmp);
			break;
		case SCHEDULE_ACTION_STATUS:
			tmp = xmlnode_get_data(action);
			purple_schedule_add_action(schedule, type, tmp);
			g_free(tmp);
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
			xmlnode_set_attrib(gchild, "prpl", purple_account_get_protocol_id(action->d.send.account));
			xmlnode_set_attrib(gchild, "name", purple_account_get_username(action->d.send.account));
			xmlnode_set_attrib(gchild, "who", action->d.send.who);

			gchild = xmlnode_new_child(child, "message");
			xmlnode_insert_data(gchild, action->d.send.message, -1);
			break;
		default:
			purple_debug_warning("purple-schedule", "unknown action type\n");
			break;
	}

	return node;
}

static xmlnode*
when_to_xmlnode(PurpleSchedule *schedule)
{
	xmlnode *node;

	node = xmlnode_new("when");

	xmlnode_set_attrib_int(node, "type", schedule->type);

	switch (schedule->type)
	{
		case PURPLE_SCHEDULE_TYPE_DATE:
			xmlnode_set_attrib_int(node, "date", schedule->d.date);
			break;
		case PURPLE_SCHEDULE_TYPE_DAY:
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
schedule_to_xmlnode(PurpleSchedule *schedule)
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

	node = xmlnode_new("purple-schedule");
	xmlnode_set_attrib(node, "version", PP_VERSION);

	child = xmlnode_new_child(node, "schedules");

	for (iter = schedules; iter; iter = iter->next)
	{
		PurpleSchedule *schedule = iter->data;

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
	purple_util_write_data_to_file("schedules.xml", data, -1);
	g_free(data);
	xmlnode_free(node);
}


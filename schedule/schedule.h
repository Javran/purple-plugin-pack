/*
 * Purple-Schedule - Schedule reminders/pounces at specified times.
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

#ifdef HAVE_CONFIG_H
# include "../pp_config.h"
#endif

#include <glib.h>

/* Pack/Local headers */
#include "../common/pp_internal.h"

#include <account.h>
#include <debug.h>

typedef struct _PurpleSchedule PurpleSchedule;
typedef struct _ScheduleAction ScheduleAction;

typedef enum
{
	SCHEDULE_ACTION_NONE   = 0,			/* Vegetable Schedule */
	SCHEDULE_ACTION_POPUP  = 1 << 0,	/* Popup a dialog */
	SCHEDULE_ACTION_CONV   = 1 << 1,	/* Write in conversations */
	SCHEDULE_ACTION_STATUS = 1 << 3,	/* Set status [message] to something */
} ScheduleActionType;

typedef enum
{
	PURPLE_SCHEDULE_TYPE_DATE,				/* Schedule for a date */
	PURPLE_SCHEDULE_TYPE_DAY					/* Schedule for a day (eg. Sunday) */
} ScheduleType;

typedef enum
{
	PURPLE_SCHEDULE_DAY_ALL,
	PURPLE_SCHEDULE_DAY_SUN,
	PURPLE_SCHEDULE_DAY_MON,
	PURPLE_SCHEDULE_DAY_TUE,
	PURPLE_SCHEDULE_DAY_WED,
	PURPLE_SCHEDULE_DAY_THR,
	PURPLE_SCHEDULE_DAY_FRI,
	PURPLE_SCHEDULE_DAY_SAT
} ScheduleDay;

struct _PurpleSchedule
{
	ScheduleType type;
	char *name;

	union
	{
		int date;
		ScheduleDay day;
	} d;
	int month;			/* 0 for every month */
	int year;			/* 0 for every year */
	int hour;
	int minute;
	time_t timestamp;

	gboolean active;	/* we can deactivate some scheduler */

	GList *actions;		/* A list of ScheduleActions */
};

struct _ScheduleAction
{
	ScheduleActionType type;

	union
	{
		char *popup_message;	/* popup */
		struct
		{
			char *message;		/* send message */
			char *who;
			PurpleAccount *account;
		} send;
		char *status_title;		/* title of the status to change to */
	} d;
};

PurpleSchedule *purple_schedule_new();

void purple_schedule_destroy(PurpleSchedule *schedule);

void purple_schedule_add_action(PurpleSchedule *schedule, ScheduleActionType type, ...);

void purple_schedule_remove_action(PurpleSchedule *schedule, ScheduleActionType type);

void purple_schedule_action_destroy(ScheduleAction *action);

GList *purple_schedules_get_all();

void purple_schedules_add(PurpleSchedule *schedule);

void purple_schedule_reschedule(PurpleSchedule *schedule);

void purple_schedule_init();

void purple_schedule_uninit();

void purple_schedules_sync();

G_END_DECLS


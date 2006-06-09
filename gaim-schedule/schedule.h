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

#include <glib.h>

/* Pack/Local headers */
#include "../common/i18n.h"

#include <account.h>
#include <debug.h>

typedef struct _GaimSchedule GaimSchedule;
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
	GAIM_SCHEDULE_TYPE_DATE,				/* Schedule for a date */
	GAIM_SCHEDULE_TYPE_DAY					/* Schedule for a day (eg. Sunday) */
} ScheduleType;

typedef enum
{
	GAIM_SCHEDULE_DAY_ALL,
	GAIM_SCHEDULE_DAY_SUN,
	GAIM_SCHEDULE_DAY_MON,
	GAIM_SCHEDULE_DAY_TUE,
	GAIM_SCHEDULE_DAY_WED,
	GAIM_SCHEDULE_DAY_THR,
	GAIM_SCHEDULE_DAY_FRI,
	GAIM_SCHEDULE_DAY_SAT
} ScheduleDay;

struct _GaimSchedule
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
			GaimAccount *account;
		} send;
		char *status_title;		/* title of the status to change to */
	} d;
};

GaimSchedule *gaim_schedule_new();

void gaim_schedule_destroy(GaimSchedule *schedule);

void gaim_schedule_add_action(GaimSchedule *schedule, ScheduleActionType type, ...);

void gaim_schedule_remove_action(GaimSchedule *schedule, ScheduleActionType type);

void gaim_schedule_action_destroy(ScheduleAction *action);

GList *gaim_schedules_get_all();

void gaim_schedules_add(GaimSchedule *schedule);

void gaim_schedule_reschedule(GaimSchedule *schedule);

void gaim_schedule_init();

void gaim_schedule_uninit();

void gaim_schedules_sync();

G_END_DECLS


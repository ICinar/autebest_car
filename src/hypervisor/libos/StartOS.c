/*
 * StartOS.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-08-07: initial
 */

#include <Os_Api.h>
#include <Os_PrivateCfg.h>
#include <hv.h>
#include "os_private.h"

AppModeType __OsApplicationMode;

static void Os_StartTasks(const Os_AutoStartTask_t * tasks, uint8 numtasks)
{
	unsigned i;
	for (i = 0; i < numtasks; i++)
	{
		(void) ActivateTask(tasks->id);
		tasks++;
	}
}

static void Os_StartAlarms(const Os_AutoStartAlarm_t * alarms, uint8 numalarms)
{
	unsigned i;
	for (i = 0; i < numalarms; i++)
	{
		if (alarms->isrelative)
		{
			(void) SetRelAlarm(alarms->id, alarms->alarmtime, alarms->cycletime);
		}
		else
		{
			(void) SetAbsAlarm(alarms->id, alarms->alarmtime, alarms->cycletime);
		}
		alarms++;
	}
}

static void Os_StartSchedules(const Os_AutoStartSchedule_t * schedules, uint8 numschedules)
{
	unsigned i;
	for (i = 0; i < numschedules; i++)
	{
		switch (schedules->startmode)
		{
		case OS_SCHEDAUTOMODE_RELATIVE:
			(void) StartScheduleTableRel(schedules->id, schedules->startvalue);
			break;
		case OS_SCHEDAUTOMODE_ABSOLUTE:
			(void) StartScheduleTableAbs(schedules->id, schedules->startvalue);
			break;
		case OS_SCHEDAUTOMODE_SYNCHRON:
			StartScheduleTableSynchron(schedules->id);
			break;
		default:
			/* FIXME: error handling! */
			break;
		}
		schedules++;
	}
}

void StartOS(AppModeType appmode)
{
	const Os_AppmodeConfig_t * appmodeCfg;

	__OsApplicationMode = appmode;

	if (appmode >= Os_numAppmodes)
	{
		ShutdownOS(E_OS_ID);
	}
	else
	{
		/* do autostarts */
		appmodeCfg = &Os_AppmodeConfig[appmode];
		Os_StartTasks(appmodeCfg->tasks, appmodeCfg->numtasks);
		Os_StartAlarms(appmodeCfg->alarms, appmodeCfg->numalarms);
		Os_StartSchedules(appmodeCfg->schedules, appmodeCfg->numschedules);
	}

	sys_part_set_operating_mode(PART_OPERATING_MODE_NORMAL);
	/* terminates the caller */
	sys_task_terminate();
}

/*
 * Os_PrivateCfg.h
 *
 * AUTOSAR OS library configuration interface
 *
 * This is an internal header for the OS implementation; OS users should include Os.h only.
 * This header should never be included by user code.
 *
 * tjordan, 2014-09-02: initial
 */

#ifndef OS_PRIVATE_CFG_H
#define OS_PRIVATE_CFG_H

#include <Os_Types.h>

/* Task IDs */
extern const uint8 Os_firstISRID;
extern const uint8 Os_firstHookID;

/* Resources */

typedef struct {
	uint32 lockedBy;
	ResourceType prevResource;
	uint8 prio;
	uint8 oldPrio;
} Os_ResourceData_t;

extern const uint8 Os_numResources;
extern ResourceType Os_lastResTaken[];
extern Os_ResourceData_t Os_ResourceData[];

/* Hooks */

typedef struct {
	// FIXME: do we need?
	TaskType startuphook;
	TaskType errorhook;
} Os_HookConfig_t;

extern const Os_HookConfig_t Os_HookConfig;

/* Stacks */

typedef struct {
	uint32 * stack;
	uint32 size;
} Os_StackConfig_t;

extern const uint8 Os_numStacks;
extern const Os_StackConfig_t Os_StackConfig[];

/* Autostarts */

typedef struct {
	AlarmType id;
	TickType alarmtime;
	TickType cycletime;
	boolean isrelative;
} Os_AutoStartAlarm_t;

typedef struct {
	ScheduleTableType id;
	TickType startvalue;
	char startmode;
} Os_AutoStartSchedule_t;

#define OS_SCHEDAUTOMODE_RELATIVE 'r'
#define OS_SCHEDAUTOMODE_ABSOLUTE 'a'
#define OS_SCHEDAUTOMODE_SYNCHRON 's'

typedef struct {
	TaskType id;
} Os_AutoStartTask_t;

typedef struct {
	const Os_AutoStartAlarm_t * alarms;
	const Os_AutoStartSchedule_t * schedules;
	const Os_AutoStartTask_t * tasks;
	uint8 numalarms;
	uint8 numschedules;
	uint8 numtasks;
} Os_AppmodeConfig_t;

extern const uint8 Os_numAppmodes;
extern const Os_AppmodeConfig_t Os_AppmodeConfig[];

#endif

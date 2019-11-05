/*
 * alarm_state.h
 *
 * OSEK alarm state representation
 *
 * azuepke, 2014-06-05: initial
 */

#ifndef __ALARM_STATE_H__
#define __ALARM_STATE_H__

#include <stdint.h>
#include <hv_types.h>

/** upper limit of alarms in the system (so we can use 16-bit indices) */
#define MAX_ALARMS	65536

/* Alarm states */
#define ALARM_STATE_IDLE	0
#define ALARM_STATE_ACTIVE	1

/* Alarm actions */
#define ALARM_ACTION_EVENT		0	/* set event */
#define ALARM_ACTION_TASK		1	/* activate task */
#define ALARM_ACTION_HOOK		2	/* activate hook */
#define ALARM_ACTION_INVOKE		3	/* invoke in-kernel callback */
#define ALARM_ACTION_COUNTER	4	/* increment other counter */
#define ALARM_ACTION_SCHEDTAB	5	/* schedule table action */

/* forward declarations */
struct alarm;
struct counter_cfg;
struct task;
struct schedtab;

/** static alarm configuration */
struct alarm_cfg {
	/** Associated counter */
	uint8_t counter_id;
	/** Alarm action */
	uint8_t action;
	/** Associated processor */
	uint8_t cpu_id;
	/** associated event bit for ALARM_ACTION_EVENT */
	uint8_t event_bit;
	union {
		/** associated task for ALARM_ACTION_TASK or ALARM_ACTION_HOOK */
		struct task *task;
		/** associated in-kernel callback for ALARM_ACTION_INVOKE */
		void (*alarm_callback)(void);
		/** counter to increment for ALARM_ACTION_COUNTER */
		const struct counter_cfg *counter_cfg;
		/** associated scheduling table for ALARM_ACTION_SCHEDTAB */
		struct schedtab *schedtab;
	} u;
};

/** runtime alarm data */
struct alarm {
	/** pointer to next alarm (for queuing to counters) */
	struct alarm *next;

	/** expiry time and cycle from sys_alarm_set_rel/abs() */
	ctrtick_t expiry;
	ctrtick_t cycle;

	/** associated alarm ID (constant over life time) */
	uint16_t alarm_id;
	/** Associated counter (constant over life time) */
	uint8_t counter_id;
	/** Alarm state */
	uint8_t state;
};

#endif

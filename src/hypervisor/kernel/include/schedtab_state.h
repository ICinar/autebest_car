/*
 * schedtab_state.h
 *
 * Scheduling tables (schedtab) state representation
 *
 * azuepke, 2014-06-09: initial
 */

#ifndef __SCHEDTAB_STATE_H__
#define __SCHEDTAB_STATE_H__

#include <stdint.h>
#include <hv_types.h>

/* forward declarations */
struct task;
struct alarm;

/** upper limit of schedtabs in the system (so we can use 8-bit indices) */
#define MAX_SCHEDTABS	256

/** upper limit of schedtab_actions in the system (so we can use 16-bit indices) */
#define MAX_SCHEDTAB_ACTIONS	65536

/* Scheduling Table flags */
#define SCHEDTAB_FLAG_REPEATING	0x01	/**< repeating schedule table */
/* AUTOSAR defines "OsScheduleTblSyncStrategy", quoting ECUC_Os_00065:
 * EXPLICIT: The schedule table is driven by an OS counter but processing
 *           needs to be synchronized with a different counter
 *           which is not an OS counter object.
 * IMPLICIT: The counter driving the schedule table is the counter
 *           with which synchronisation is required.
 * NONE:     No support for synchronisation.
 */
#define SCHEDTAB_FLAG_SYNC_EXPLICIT	0x02	/**< external synchronization required */
#define SCHEDTAB_FLAG_SYNC_IMPLICIT	0x04	/**< synchronization to counter required */

#define SCHEDTAB_SYNC_FLAGS (SCHEDTAB_FLAG_SYNC_EXPLICIT | SCHEDTAB_FLAG_SYNC_IMPLICIT)

/** static scheduling table configuration */
struct schedtab_cfg {
	/** Associated counter */
	uint8_t counter_id;
	/** Flags */
	uint8_t flags;
	/** start index in schedtab_action table */
	uint16_t start_idx;
	/** Associated alarm object */
	struct alarm *alarm;
	/** duration of the scheduling table */
	ctrtick_t duration;
	/** precision bound of an explicitly synchronized scheduling table */
	ctrtick_t precision;
};


/* Scheduling Table actions
 *
 * The schedule table actions are programmed in a kind of assembly language
 * comprising actions and alterations of the control flow.
 * Actions are "activate a task" or "set an event", but also set the
 * adjustments for explicit synchronization or encode relative wait times.
 *
 * Time offsets are given in 32-bit quantities in the configuration,
 * but never appear at the same time as task/hook references, so we share
 * the space.
 *
 * For any given expiry point, the tool needs to emit any SHORTEN or LENGTHEN
 * actions before the WAIT action.
 *
 * Additionally, all schedule tables start with a START element.
 */
#define SCHEDTAB_ACTION_EVENT		0	/* set event "event_bit" to task "task" */
#define SCHEDTAB_ACTION_TASK		1	/* activate task "task" */
#define SCHEDTAB_ACTION_HOOK		2	/* activate hook "task" */
#define SCHEDTAB_ACTION_WAIT		3	/* wait relative "time" */
#define SCHEDTAB_ACTION_SHORTEN		4	/* OsScheduleTableMaxShorten "time" */
#define SCHEDTAB_ACTION_LENGTHEN	5	/* OsScheduleTableMaxLengthen "time" */
#define SCHEDTAB_ACTION_WRAP		6	/* wrap-around: continue at entry "next_idx" */
#define SCHEDTAB_ACTION_START		7	/* start indicator (no arguments) */

/** static scheduling table configuration */
struct schedtab_action_cfg {
	/** encoded action */
	uint8_t action;
	/** associated event bit for SCHEDTAB_ACTION_EVENT */
	uint8_t event_bit;
	/** follow up action for SCHEDTAB_ACTION_WRAP */
	uint16_t next_idx;
	union {
		/** associated task for various actions */
		struct task *task;
		/** time value for time-specific actions */
		uint32_t time;
	} u;
};


/** runtime scheduling table data */
struct schedtab {
	/** associated static entry (constant over life time) */
	uint8_t schedtab_id;
	/** Schedule table state */
	uint8_t state;
	/** current index in schedtab_action table */
	uint16_t current_idx;

	/** next schedule table (set to self for repeating schedule tables) */
	struct schedtab *next;

	/** offset of our "drive counter" value at start of the schedule table
	 *
	 *  current position in the schedule table:
	 *    current_schedtab_offset = current_drivecounter_value - sync_counter_offset
	 */
	ctrtick_t sync_counter_offset;

	/** current deviation of schedule table (signed value):
	 *
	 *  deviation = synchronization_time - current_schedtab_offset
	 *
	 *  deviation == 0  ->  perfectly in sync:
	 *  deviation < 0   ->  schedule table runs too fast, needs to lengthen
	 *  deviation > 0   ->  schedule table runs too slow, needs to shorten
	 */
	ctrphase_t deviation;
};

#endif

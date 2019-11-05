/*
 * counter_state.h
 *
 * OSEK counters
 *
 * azuepke, 2014-06-05: initial
 */

#ifndef __COUNTER_STATE_H__
#define __COUNTER_STATE_H__

#include <stdint.h>
#include <hv_types.h>

/** upper limit of counters in the system (so we can use 8-bit indices) */
#define MAX_COUNTERS	256

/* Counter types */
#define COUNTER_TYPE_SW	0
#define COUNTER_TYPE_HW	1

/* forward declarations */
struct alarm;
struct counter;
struct counter_cfg;

/** static counter configuration */
struct counter_cfg {
	/** associated runtime data */
	struct counter *counter;

	/** Register a counter at boot time */
	void (*reg)(const struct counter_cfg *ctr_cfg);
	/** Accessor function to read or write a hardware counter */
	ctrtick_t (*query)(const struct counter_cfg *ctr_cfg);
	void (*change)(const struct counter_cfg *ctr_cfg);

	/** The maximum value of a counter in ticks. */
	ctrtick_t maxallowedvalue;
	/** Number of ticks of a counter to reach a significant "unit". */
	ctrtick_t ticksperbase;
	/** The minimum value for a cycle. */
	ctrtick_t mincycle;

	/** Counter type */
	uint8_t type;
	/** Associated processor */
	uint8_t cpu_id;
	uint16_t padding;
};

/** runtime counter data */
struct counter {
	/** ordered single linked list of queued alarms (or NULL if empty) */
	struct alarm *first;

	/** current counter value */
	ctrtick_t current;
};


/** static counter access configuration of the partitions */
struct counter_access {
	/** Related counter */
	const struct counter_cfg *counter_cfg;
};

#endif

/*
 * hv_types.h
 *
 * Kernel API specific types.
 *
 * azuepke, 2013-11-25: initial version for MPU kernel
 * azuepke, 2015-06-22: added full API documentation
 */

#ifndef __HV_TYPES_H__
#define __HV_TYPES_H__

#include <stdint.h>

/** System time in nanoseconds
 *
 * The kernel uses an unsigned 64 bit nanosecond data type for the system time.
 * The time starts at 0, which relates to the system boot time.
 * The effective resolution is 63 bits and lasts for about 292 years.
 */
typedef uint64_t time_t;

/** Timeout in nanoseconds
 *
 * The kernel uses a signed 64 bit nanosecond data type for timeout.
 * The effective resolution of 63 bits lasts for about 292 years:
 * - positive numbers are used for relative timeouts in nanoseconds
 * - a timeout value of zero indicates a null timeout that never blocks
 * - negative numbers indicate infinity
 *
 * \see INFINITY
 */
typedef int64_t timeout_t;

/** Event mask
 *
 * Each extended task has a 32 bit event mask.
 */
typedef uint32_t evmask_t;

/** User scheduling state
 *
 * The user scheduling state is part of the user <-> kernel protocol
 * for fast prio switching, which again is used for fast resource handling.
 *
 * User code can change \a user_prio at will: The kernel synchronizes this
 * value with its internal priority representation at the time a scheduling
 * decision happens (e.g. a task is placed on the ready queue).
 * The kernel bounds this value to the partition's maximum configured priority.
 *
 * A violation of this bound is a user error that does not affect partition
 * scheduling.
 *
 * The kernel updates \a next_prio to reflect the priority of the next task
 * to schedule.
 *
 * Lastly, \a taskid contains the current task ID.
 */
typedef struct {
	/** Current Task ID */
	uint16_t taskid;
	/** Current Task's Priority in user space */
	uint8_t user_prio;
	/** Next Pending Task's Priority in the scheduler */
	uint8_t next_prio;
} user_sched_state_t;


/** Counter value
 *
 * The kernel uses an unsigned 32 bit data type for counter values.
 */
typedef uint32_t ctrtick_t;

/** Counter phase
 *
 * The kernel uses a signed 32 bit data type for counter phases.
 * In relation to another counter, a phase value expresses:
 * - a negative value expresses a phase ahead of a related counter
 * - a positive value expresses a phase behind a related counter
 * - a zero value expresses that phases are synchronous
 */
typedef int32_t ctrphase_t;

/** Wait queue queuing discipline */
#define WQ_DISCIPLINE_FIFO	0
#define WQ_DISCIPLINE_PRIO	1

/** Scheduling table states */
#define SCHEDTAB_STATE_STOPPED			0
#define SCHEDTAB_STATE_NEXT				1
#define SCHEDTAB_STATE_WAITING			2
#define SCHEDTAB_STATE_RUNNING			3
#define SCHEDTAB_STATE_RUNNING_SYNC		4
#define SCHEDTAB_STATE_RUNNING_ASYNC	5

/** Partition operating modes */
#define PART_OPERATING_MODE_IDLE		0
#define PART_OPERATING_MODE_COLD_START	1
#define PART_OPERATING_MODE_WARM_START	2
#define PART_OPERATING_MODE_NORMAL		3

/** Partition start conditions */
#define PART_START_CONDITION_NORMAL_START			0
#define PART_START_CONDITION_PARTITION_RESTART		1
#define PART_START_CONDITION_HM_MODULE_RESTART		2
#define PART_START_CONDITION_HM_PARTITION_RESTART	3

/** Task states */
#define TASK_STATE_SUSPENDED	0	/**< Task suspended / terminated */
#define TASK_STATE_WAIT_ACT		1	/**< Task waiting for activation (delayed start) */
#define TASK_STATE_WAIT_WQ		2	/**< Task waiting on a wait queue */
#define TASK_STATE_WAIT_EV		3	/**< Task waiting for an event */
#define TASK_STATE_WAIT_SEND	4	/**< Task waiting for RPC send */
#define TASK_STATE_WAIT_RECV	5	/**< Task waiting for RPC recv */
#define TASK_STATE_READY		6	/**< Ready, task on the ready queue */
#define TASK_STATE_RUNNING		7	/**< Current task, not on ready queue */

/** Error state
 *
 * This data structure is part of the user <-> kernel protocol
 * for asynchronous error handling.
 * Element \a task_id refers to the faulting task, with error code \a error_code
 * explaining the error.
 * Additional information can be supplied in \a extra.
 */
typedef struct {
	/** Partition local task ID. */
	uint16_t task_id;
	/** Error code. */
	uint8_t error_code;
	uint8_t padding;
	/** Extra information, e.g. event bit in SetEvent() */
	unsigned long extra;
} user_error_state_t;

/** Exception state
 *
 * This data structure is part of the user <-> kernel protocol
 * for synchronous exception handling.
 * Element \a task_id refers to the faulting task, with error code \a error_code
 * explaining the exception condition.
 * An architecture dependent fault address is provided in \a fault_addr.
 */
typedef struct {
	/** Partition local task ID. */
	uint16_t task_id;
	/** Error code. */
	uint8_t error_code;
	uint8_t padding;
	/** Architecture dependent fault address. */
	unsigned long fault_addr;
} user_exception_state_t;

/** Halt mode
 *
 * Modes to halt or reset the system.
 */
typedef enum {
	/* regular halt, reset, or shutdown the board on user request */
	BOARD_HALT,
	BOARD_RESET,
	BOARD_SHUTDOWN,
	/* reset or shutdown the board due to health monitor action */
	BOARD_HM_RESET,
	BOARD_HM_SHUTDOWN,
	/* assert triggered in development kernel */
	BOARD_HM_ASSERT,
} haltmode_t;

#endif

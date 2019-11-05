/*
 * part_state.h
 *
 * Kernel partition state.
 *
 * azuepke, 2013-11-24: initial MPU version
 */

#ifndef __PART_STATE_H__
#define __PART_STATE_H__

#include <stdint.h>
#include <hv_types.h>

/* forward declaration */
struct arch_mpu_part_cfg;
struct counter_access;
struct shm_access;
struct kldd_cfg;
struct ipev_cfg;
struct schedtab;
struct alarm;
struct part;
struct task;
struct wq_cfg;
struct wq;
struct rpc_cfg;

/** upper limit of partitions (so we can use 8-bit indices) */
#define MAX_PARTITIONS	256

/* partition flags */
#define PART_FLAG_PRIVILEGED			0x00000001
#define PART_FLAG_RESTARTABLE			0x00000002

/** Number of memory ranges */
#define NUM_MEM_RANGES	4

/** A memory range. An address is valid if start <= addr < end. */
struct mem_range {
	addr_t start;
	addr_t end;
};

/** static partition configuration:
 */
struct part_cfg {
	/* pointer to runtime data */
	struct part *part;

	/* pointer to partition's tasks */
	struct task *tasks;

	/* pointer to partition's alarms */
	struct alarm *alarms;

	/* pointer to partition's schedule tables */
	struct schedtab *schedtabs;

	/* pointer to partition's wait queues */
	const struct wq_cfg *wq_cfgs;
	struct wq *wqs;

	/* pointer to partition's accessible counters */
	const struct counter_access *ctr_accs;

	/* pointer to partition's accessible SHMs */
	const struct shm_access *shm_accs;

	/* pointer to partition's KLDDs */
	const struct kldd_cfg *kldds;

	/* pointer to partition's inter-partition events */
	const struct ipev_cfg *ipevs;

	/* partition specific MPU config */
	const struct arch_mpu_part_cfg *mpu_part_cfg;

	/* associated RPCs */
	const struct rpc_cfg *rpcs;

	/* partition scheduling */
	time_t period;
	time_t duration;		/* for ARINC information purposes */

	/* scheduling data in user space */
	user_sched_state_t *user_sched_state;
	/* error handling data in user space (array with 1..256 entries) */
	user_error_state_t *user_error_state;
	/* exception handling data in user space */
	user_exception_state_t *user_exception_state;

	/* related objects */
	uint16_t num_tasks;
	uint8_t num_kldds;
	uint8_t num_schedtabs;
	uint16_t num_ipevs;
	uint16_t num_ctr_accs;
	uint16_t num_alarms;
	uint16_t num_wqs;
	uint8_t num_shm_accs;
	uint8_t cpu_id;			/* associated processor */
	uint8_t num_ctxts;		/* number of contexts in save area */
	uint8_t tp_id;			/* associated time partition */

	uint16_t num_rpcs;
	uint16_t padding;

	uint16_t init_hook_id;	/* partition startup hook (local task ID, always used) */
	uint16_t error_hook_id;	/* error and protection hook (local task ID, 0xffff if not used) */
	uint16_t exception_hook_id;	/* exception hook (local task ID, 0xffff if not used) */
	uint16_t num_error_states;	/* number of entries in the error state array */

	/* attributes */
	uint8_t initial_operating_mode;
	uint8_t part_id;
	uint8_t max_prio;
	uint8_t flags;

	struct mem_range mem_ranges[NUM_MEM_RANGES];

	const char *name;

	/* addresses of small data areas */
	addr_t sda1_base;
	addr_t sda2_base;
};

/** runtime partition data
 * - each task also has an assigned register frame (entry part_id)
 */
struct part {
	/** pointer to static partition configuration */
	const struct part_cfg *cfg;

	/** idle, cold_start, warm_start, normal */
	uint8_t operating_mode;
	/** partition was once in normal (ARINC) */
	uint8_t warm_startable;
	/** partition start condition (ARINC) */
	uint8_t start_condition;
	/** current position in the error handling array */
	uint8_t error_write_pos;

	/** if non-zero, the partition has a pending operationg mode change */
	uint8_t pending_mode_change;
	/** new operating mode to enter */
	uint8_t new_operating_mode;
	uint16_t padding;

	/** single linked list: partitions with pending mode changes */
	struct part *next_pending_mode_change;

	/** last scheduled real task (may be current one or NULL for idle) */
	struct task *last_real_task;
};

#endif

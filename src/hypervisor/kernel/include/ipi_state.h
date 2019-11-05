/*
 * ipi_state.h
 *
 * Multicore IPI-queue state.
 *
 * azuepke, 2015-03-30: initial
 */

#ifndef __IPI_STATE_H__
#define __IPI_STATE_H__

#include <stdint.h>
#include <sched_state.h>

/** Limit number of IPI actions per core so we can use 8-bit indices */
#define MAX_IPI_ACTIONS 256

/* forward */
struct ipi_action;
struct ipi_state;
struct task;
struct counter_cfg;
struct schedtab;
struct part;
struct wq;
struct tpschedule_cfg;


/** per-core IPI config */
struct ipi_cfg {
	/** each core has a job-queue for each other core */
	struct ipi_action *actions[MAX_CPUS];

	/** IPI state */
	struct ipi_state *state;
};

/** per-core IPI state */
struct ipi_state {
	/** outgoing jobs: write-position in local queue, modulo num_ipi_actions */
	uint8_t write_pos[MAX_CPUS];

	/** incoming jobs: read-position in foreign queue, modulo num_ipi_actions */
	uint8_t read_pos[MAX_CPUS];
};

/** IPI action types */
#define IPI_ACTION_EVENT		0	/* set event "aux" to task "task" */
#define IPI_ACTION_TASK			1	/* activate task "task" */
#define IPI_ACTION_HOOK			2	/* activate hook "task" */
#define IPI_ACTION_WQ_WAKE		3	/* wait tasks on wait queue */
#define IPI_ACTION_COUNTER		4	/* increment other counter */
#define IPI_ACTION_PART_STATE	5	/* partition state change */
#define IPI_ACTION_SCHEDULE_CHANGE	6	/* change time partition schedule */
#define IPI_ACTION_EVENT_NOERR	7	/* set event "aux" to task "task", no error */

/** IPI action */
struct ipi_action {
	/** IPI action */
	uint8_t action;
	/** associated event bit for IPI_ACTION_EVENT or partition/core mode */
	uint8_t aux;
	uint16_t padding;
	union {
		/** associated task for EVENT, TASK, or HOOK call */
		struct task *task;
		/** counter to increment for IPI_ACTION_COUNTER */
		const struct counter_cfg *counter_cfg;
		/** associated scheduling table for IPI_ACTION_SCHEDTAB */
		struct schedtab *schedtab;
		/** associated partition for IPI_ACTION_PART_STATE */
		struct part *part;
		/** associated wait queue for IPI_ACTION_WQ_WAKE */
		struct wq *wq;
		/** next time partition schedule table */
		const struct tpschedule_cfg *next_tpschedule;
		/** generic pointer */
		const void *object;
	} u;
};

#endif

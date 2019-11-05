/*
 * config.h
 *
 * APEX internal configuration
 *
 * azuepke, 2014-09-08: initial
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <ARINC653.h>
#include <hv_types.h>

/* partition attributes */
extern const uint64_t __apex_part_period;
extern const uint64_t __apex_part_duration;

extern const uint8_t __apex_part_max_prio;

/* processes */
struct apex_proc_cfg {
	/* NOTE: NUL-terminated string */
	char name[32];

	uint32_t entry_point;
	uint16_t stack_size;
	uint8_t deadline_type;
	uint8_t base_prio;

	timeout_t period;
	timeout_t capacity;
};

extern const uint16_t __apex_init_hook_id;

extern const uint16_t __apex_num_procs;
extern const struct apex_proc_cfg __apex_proc_cfg[];

#define PROC_STATE_DEAD			0	/* not created */
#define PROC_STATE_NORMAL		1	/* not in critical section */
#define PROC_STATE_CRIT			2	/* inside critical section */
#define PROC_STATE_CRIT_SUSP	3	/* inside critical section with suspension request */
#define PROC_STATE_SUSP_SLEEP	4	/* suspended (sleeping) */
#define PROC_STATE_SUSP_PRIO0	5	/* suspended (prio 0) */
#define PROC_STATE_DORMANT		6	/* dormant / terminated */

extern uint8_t __apex_proc_state[];
/* NOTE: __apex_proc_prio also includes all hooks */
extern uint8_t __apex_proc_prio[];

/* error handler */
extern const uint32_t __apex_error_handler_entry_point;
extern const uint16_t __apex_error_handler_stack_size;
extern const uint16_t __apex_error_handler_hook_id;


/* semaphores */
struct apex_sem_cfg {
	/* NOTE: NUL-terminated string */
	char name[32];

	uint16_t wq_id;
};

#define SEM_INVALID		0x0000
#define SEM_MAX_MASK	0x7fff
#define SEM_DISC_PRIO	0x8000
struct apex_sem {
	uint16_t disc_max;	/* bit 15 encodes Priority discipline, lower bits MAX */
	int16_t val;		/* <0: waiters, >=0: count */
};

extern const uint16_t __apex_num_sems;
extern const struct apex_sem_cfg __apex_sem_cfg[];
extern struct apex_sem __apex_sem_dyn[];



/* queuing ports */
struct qport_messages {
	volatile uint32_t state;
	char messages[];
};

struct apex_qport_cfg {
	/** messages (max_nb_message * max_message_size bytes, if connected) */
	void *m;

	/** max number of messages */
	uint32_t max_nb_message;
	/** max message size in bytes */
	size_t max_message_size;

	/* associated wait queue */
	uint16_t wq_id;

	/** direction of the queuing port */
	uint8_t port_direction;
	uint8_t FIXME_unused_alignment;

	SAMPLING_PORT_NAME_TYPE name;
};

struct apex_qport {
	/** pointer to static configuration */
	const struct qport_cfg *cfg;

	/** state (0 == closed, 1 == created) */
	uint8_t state;
	/** queuing discipline */
	uint8_t queuing_discipline;

	/** number of waiting tasks */
	uint16_t waiting;
};

extern const unsigned int __apex_num_qports;
extern const struct apex_qport_cfg __apex_qport_cfg[];
extern struct apex_qport __apex_qport_dyn[];

#endif

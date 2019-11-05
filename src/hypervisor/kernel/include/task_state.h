/*
 * task_state.h
 *
 * Kernel task state.
 *
 * azuepke, 2013-11-24: initial MPU version
 * azuepke, 2013-12-09: reworked task states
 */

#ifndef __TASK_STATE_H__
#define __TASK_STATE_H__

#include <stdint.h>
#include <hv_types.h>
#include <list.h>

/* forward declaration */
struct task;
struct part_cfg;
struct arch_reg_frame;
struct arch_fpu_frame;
struct arch_ctxt_frame;
struct arch_mpu_task_cfg;
struct timepart_state;
struct rpc;

/** upper limit of tasks in the system (we use 16-bit indices) */
#define MAX_TASKS	65536

/** upper limit of task activations for basic tasks */
#define MAX_TASK_ACTIVATIONS	255


/** Task states in the scheduler:
 *  Task states are encoded in task::flags_state together with task flags.
 */
/* see hv_types.h */
#define TASK_STATE_MASK				0x07
#define TASK_STATE(x)				((x) & TASK_STATE_MASK)
#define TASK_SET_STATE(x,s)			(((x) & ~TASK_STATE_MASK) | (s))


/** Test task states */
#define TASK_STATE_IS_RUNNING(x)	(((x) & TASK_STATE_MASK) == TASK_STATE_RUNNING)
#define TASK_STATE_IS_READY(x)		(((x) & TASK_STATE_MASK) == TASK_STATE_READY)
#define TASK_STATE_IS_WAIT_RECV(x)	(((x) & TASK_STATE_MASK) == TASK_STATE_WAIT_RECV)
#define TASK_STATE_IS_WAIT_SEND(x)	(((x) & TASK_STATE_MASK) == TASK_STATE_WAIT_SEND)
#define TASK_STATE_IS_WAIT_EV(x)	(((x) & TASK_STATE_MASK) == TASK_STATE_WAIT_EV)
#define TASK_STATE_IS_WAIT_WQ(x)	(((x) & TASK_STATE_MASK) == TASK_STATE_WAIT_WQ)
#define TASK_STATE_IS_WAIT_ACT(x)	(((x) & TASK_STATE_MASK) == TASK_STATE_WAIT_ACT)
#define TASK_STATE_IS_SUSPENDED(x)	(((x) & TASK_STATE_MASK) == TASK_STATE_SUSPENDED)

/** Task types:
 *  Task types are kept in task_cfg::cfgflags_type together with task config flags.
 */
#define TASK_TYPE_HOOK				0x00	/* task is a hook */
#define TASK_TYPE_ISR				0x01	/* task is a cat 2 ISR */
#define TASK_TYPE_TASK				0x02	/* task is basic or extended task */
#define TASK_TYPE_INVOKABLE			0x03	/* task is an RPC call target */

#define TASK_TYPE_MASK				0x03
#define TASK_TYPE(x)				((x) & TASK_TYPE_MASK)

/** test if a task is a normal or extended task (no hook, no ISR) */
#define TASK_TYPE_IS_HOOK(x)		(((x) & TASK_TYPE_MASK) == TASK_TYPE_HOOK)
#define TASK_TYPE_IS_ISR(x)			(((x) & TASK_TYPE_MASK) == TASK_TYPE_ISR)
#define TASK_TYPE_IS_TASK(x)		(((x) & TASK_TYPE_MASK) == TASK_TYPE_TASK)
#define TASK_TYPE_IS_INVOKABLE(x)	(((x) & TASK_TYPE_MASK) == TASK_TYPE_INVOKABLE)


/** task config flags:
 *  Task config flags are encoded in task_cfg::cfgflags_type.
 */
#define TASK_CFGFLAG_ELEV_PRIO		0x80	/* elevate priority on scheduling */
#define TASK_CFGFLAG_MAYBLOCK		0x40	/* indicate that a task may block */
#define TASK_CFGFLAG_ACTIVATABLE	0x20	/* task is activatable (has entry point and stack) */
#define TASK_CFGFLAG_CALLABLE		0x10	/* task is callable via RPC */
#define TASK_CFGFLAG_ISR_UNMASK		0x08	/* ISR's interrupt source is unmasked at partition start */
#define TASK_CFGFLAG_UNUSED04		0x04	/* unused */

/** task flags:
 *  The flags are kept in task::flags_state together with the scheduling state.
 *  On a DEAD/SUSPEND -> READY transition, some task config flags are copied
 *  from ROM to RAM.
 */
#define TASK_FLAG_ELEV_PRIO			0x80	/* elevate priority on scheduling */
#define TASK_FLAG_MAYBLOCK			0x40	/* indicate that a task may block */
#define TASK_FLAG_UNUSED20			0x20	/* unused */
#define TASK_FLAG_UNUSED10			0x10	/* unused */
#define TASK_FLAG_UNUSED08			0x08	/* unused */

#define TASK_FLAG_TO_COPY			(TASK_FLAG_ELEV_PRIO | TASK_FLAG_MAYBLOCK)

/** Copy flags from ROM to RAM and set SUSPENDED state */
#define TASK_COPY_FLAGS(x)			(((x) & TASK_FLAG_TO_COPY) | TASK_STATE_SUSPENDED)

/** Test if tasks, ISRs or hooks may */
#define TASK_MAY_BLOCK(x)			((x) & TASK_FLAG_MAYBLOCK)



/** a task configuration */
struct task_cfg {
	/* pointer to runtime data */
	struct task *task;

	/* assigned user registers */
	struct arch_reg_frame *regs;

	/* assigned FPU registers */
	struct arch_fpu_frame *fpu;

	/* assigned register context save area (Tricore) */
	struct arch_ctxt_frame *ctxt;

	/* task specific MPU config */
	const struct arch_mpu_task_cfg *mpu_task_cfg;

	/* pointer to associated partition */
	const struct part_cfg *part_cfg;

	/* pointer to associated time partition */
	struct timepart_state *timepart;

	uint8_t cpu_id;			/* task's associated processor */
	uint8_t num_ctxts;		/* number of contexts in save area */
	uint16_t task_id;		/* own task ID (local to partition) */

	uint8_t cfgflags_type;	/* static task flags and task type */
	uint8_t max_activations;	/* maximum number of activations (1 for ETs) */
	uint16_t irq;			/* associated IRQ (if TASK_TYPE_ISR is set) */

	uint16_t stack_size;	/* stack size in bytes */
	/* scheduling attributes at startup */
	uint8_t base_prio;		/* base priority */
	uint8_t elev_prio;		/* elevated priority (e.g. while holding resources) */

	timeout_t period;		/* process period */
	timeout_t capacity;		/* required time capacity, relative deadline since activation */

	/* hardcoded entry points */
	unsigned long entry;
	unsigned long stack;
	unsigned long arg0;

	const char *name;

	struct rpc *rpc;		/* hook's associated RPC queue (for RPC reply) */
};

struct task {
	/** pointer to task's configuration data */
	const struct task_cfg *cfg;

	/* scheduling */
	uint8_t task_prio;		/* task's scheduling priority */
	uint8_t wait_prio;		/* priority in wait queues, RPC call priority */
	uint8_t flags_state;	/* dynamic task flags and state */
	uint8_t pending_activations;	/* basic task's pending activations */

	/* Queues and timeouts:
	 * - a single task can be either waiting on the ready queue
	 *   or in a timeout queue at the same time.
	 * - further, the task can wait on an arbitrary wait-queue
	 * - deadline monitoring is done independently to that
	 * - expiry_time denotes the time of the next wakeup
	 * - the activation_delay is set once via DELAYED_START()
	 */
	list_t ready_and_timeoutq;	/* ready queue or timeout queue node (double-linked list) */
	list_t waitq;			/* wait queue node in a double-linked list */
	time_t expiry_time;		/* timeout expiry time */
	time_t last_activation;	/* time of last planned activation */

	time_t deadline;		/* deadline expiry time */
	list_t deadlineq;		/* deadline node in a double-linked list */

	evmask_t ev_pending;	/* currently pending event */
	struct task *rpc_task;	/* associated RPC receiver (for calling task) */
};

#endif

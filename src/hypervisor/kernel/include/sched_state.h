/*
 * sched_state.h
 *
 * Kernel internal scheduler state.
 *
 * azuepke, 2013-11-24: initial MPU version
 * azuepke, 2015-04-27: time partitioning
 */

#ifndef __SCHED_STATE_H__
#define __SCHED_STATE_H__

#include <arch_state.h>		/* also used in assembler context */

#ifndef __ASSEMBLER__

#include <hv_compiler.h>
#include <stdint.h>
#include <hv_types.h>
#include <list.h>

/* forward declarations */
struct arch_reg_frame;
struct arch_fpu_frame;
struct task;
struct part_cfg;
struct part;

/** upper limit of CPUs in the system (so we can use 8-bit indices) */
#define MAX_CPUS	4

/** limit of the priority range [0..255] we encode in 8-bits */
#define NUM_PRIOS	256
/** highest possible priority */
#define MAX_PRIO	(NUM_PRIOS-1)


/** Scheduling runtime data per time partition */
struct timepart_state {
	/** ready queue root array -- a double-linked list for each prio */
	list_t readyq[NUM_PRIOS];

	/** priority tracking bitmap, one bit per prio level */
	uint32_t active_fine[NUM_PRIOS >> 5];
	uint32_t active_coarse;

	/** priority of the next schedulable task in READY state */
	uint8_t next_prio;

	/** associated time partition */
	uint8_t timepart_id;
	uint8_t padding3;
	uint8_t padding4;

	/** timeout queue -- a double-linked list */
	list_t timeoutq;
	/** deadline queue -- a double-linked list */
	list_t deadlineq;

	/** last time partition release point */
	time_t last_release_point;
};

/** a processor's internal state */
struct sched_state {
	/* NOTE: the first elements are accessed from assembler code,
	 * dont change order!
	 */
	/** architecture specific state that needs to accessed from asm code,
	 * MUST COME FIRST!
	 */
	struct arch_state arch;

	/** a partition's or task's register frame */
	struct arch_reg_frame *regs;
	/** associated FPU (if not NULL) */
	struct arch_fpu_frame *fpu;

	/** state indicator to enter the scheduler on kernel exit.
	 * on SMP, each bit represents a CPU to send an IPI to,
	 * except for the own one, where it just causes rescheduling
	 */
	uint32_t reschedule;

	/* --- order may change from here on --- */

	/** currently scheduled task or ISR */
	struct task *current_task;
	/** currently scheduled task's partition config */
	const struct part_cfg *current_part_cfg;
	/** user scheduling state */
	user_sched_state_t *user_sched_state;
	/** current time partion */
	struct timepart_state *timepart;

	/** processor idle task (does not change after initialization) */
	struct task *idle_task;

	/** single linked list: partitions with pending mode changes */
	struct part *pending_part_mode_change;

	/* time partition scheduling */

	/** Current window */
	const struct tpwindow_cfg *tpwindow;
	/** Next schedule table (checked at wrap around) */
	const struct tpwindow_cfg *wrap_tpwindow;

	/** Time of last time partition switch */
	time_t last_tp_switch;
	/** Time of next time partition switch */
	time_t next_tp_switch;
} __aligned(SCHED_STATE_ALIGN);

#endif

/** offsets into sched_state */
#define SCHED_STATE_REGS			(ARCH_STATE_SIZE + 0)
#define SCHED_STATE_FPU				(ARCH_STATE_SIZE + 4)
#define SCHED_STATE_RESCHEDULE		(ARCH_STATE_SIZE + 8)
#define SCHED_STATE_NEXT_PRIO		(ARCH_STATE_SIZE + 12)

#endif

/*
 * core_state.h
 *
 * Kernel internal per-core state.
 *
 * azuepke, 2015-03-23: initial
 */

#ifndef __CORE_STATE_H__
#define __CORE_STATE_H__

#include <hv_compiler.h>
#include <stdint.h>
#include <hv_types.h>

/* forward declarations */
struct sched_state;
struct counter_cfg;
struct core_state;

/** per-CPU configuration */
struct core_cfg {
	struct sched_state *sched;
	struct timepart_state *timeparts;
	struct core_state *core_state;

	unsigned char *kern_stack;
	unsigned char *nmi_stack;
	unsigned char *idle_stack;

	struct arch_ctxt_frame *kern_ctxts;
	struct arch_ctxt_frame *nmi_ctxts;
	struct arch_ctxt_frame *idle_ctxts;
};

/** per-CPU miscellaneous global data */
struct core_state {
	/* system timer */
	const struct counter_cfg *system_timer_ctr_cfg;
	ctrtick_t system_timer_count;

	/* HM state */
	uint8_t hm_panic_in_progress;
};

#endif

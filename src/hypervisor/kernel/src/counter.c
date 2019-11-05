/*
 * counter.c
 *
 * OSEK counters
 *
 * azuepke, 2014-06-05: initial
 */

#include <kernel.h>
#include <assert.h>
#include <counter.h>
#include <alarm.h>
#include <hv_error.h>
#include <sched.h>
#include <part.h>
#include <ipi.h>


/** initialize all counters
 * NOTE: called ONCE during system startup for each CPU
 */
__init void counter_init_all_per_cpu(void)
{
	const struct counter_cfg *ctr_cfg;
	struct counter *ctr;
	unsigned int cpu;
	unsigned int i;

	cpu = arch_cpu_id();

	/* initialize counters */
	for (i = 0; i < num_counters; i++) {
		ctr_cfg  = &counter_cfg[i];

		assert(ctr_cfg->type == COUNTER_TYPE_SW || ctr_cfg->type == COUNTER_TYPE_HW);
		assert(ctr_cfg->maxallowedvalue > 0);
		assert(ctr_cfg->ticksperbase <= ctr_cfg->maxallowedvalue);
		assert(ctr_cfg->mincycle <= ctr_cfg->maxallowedvalue);
#ifdef SMP
		/* FIXME: disable CPU check for UP builds, so we can run
		 * normal multicore demos with additional per-CPU counters.
		 */
		assert(ctr_cfg->cpu_id < num_cpus);
#endif

		if (ctr_cfg->cpu_id == cpu) {
			ctr = ctr_cfg->counter;
			assert(ctr != NULL);
			ctr->first = NULL;
			ctr->current = 0;

			if (ctr_cfg->type == COUNTER_TYPE_HW) {
				assert(ctr_cfg->reg != NULL);
				assert(ctr_cfg->query != NULL);
				assert(ctr_cfg->change != NULL);

				/* register counter callback argument in counter source */
				ctr_cfg->reg(ctr_cfg);
			}
		}
	}
}

/** Increment (software) counter by one tick */
void sys_ctr_increment(unsigned int ctr_id)
{
	const struct counter_cfg *ctr_cfg;
	const struct part_cfg *part_cfg;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);

	/* validate counter ID in this partition */
	if (ctr_id >= part_cfg->num_ctr_accs) {
		SET_RET(E_OS_ID);
		return;
	}
	ctr_cfg = part_cfg->ctr_accs[ctr_id].counter_cfg;

	if (ctr_cfg->type != COUNTER_TYPE_SW) {
		SET_RET(E_OS_ID);
		return;
	}

	SET_RET(E_OK);

#ifdef SMP
	if (ctr_cfg->cpu_id != arch_cpu_id()) {
		/* cross-core counter */
		ipi_enqueue(ctr_cfg->cpu_id, ctr_cfg, IPI_ACTION_COUNTER, 1);
		return;
	}
#endif

	kernel_increment_counter(ctr_cfg, 1);
}

/** Get current counter value in ticks */
void sys_ctr_get(unsigned int ctr_id)
{
	const struct counter_cfg *ctr_cfg;
	const struct part_cfg *part_cfg;
	ctrtick_t current;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);

	/* validate counter ID in this partition */
	if (ctr_id >= part_cfg->num_ctr_accs) {
		SET_RET(E_OS_ID);
		return;
	}
	ctr_cfg = part_cfg->ctr_accs[ctr_id].counter_cfg;

	SET_RET(E_OK);

	/* get current counter value */
#ifdef SMP
	if (ctr_cfg->cpu_id != arch_cpu_id()) {
		struct counter *ctr;

		/* cross-core counter: read cached value only */
		ctr = ctr_cfg->counter;
		current = ctr->current;
	} else
#endif
	{
		current = counter_query(ctr_cfg);
	}

	SET_OUT1(current);
}

/** Get elapsed counter value in ticks */
void sys_ctr_elapsed(unsigned int ctr_id, ctrtick_t previous)
{
	const struct part_cfg *part_cfg;
	const struct counter_cfg *ctr_cfg;
	ctrtick_t current;
	ctrtick_t elapsed;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);

	/* validate counter ID in this partition */
	if (ctr_id >= part_cfg->num_ctr_accs) {
		SET_RET(E_OS_ID);
		return;
	}
	ctr_cfg = part_cfg->ctr_accs[ctr_id].counter_cfg;

	/* validate previous */
	if (previous >= ctr_cfg->maxallowedvalue) {
		SET_RET(E_OS_VALUE);
		return;
	}

	/* get current counter value */
#ifdef SMP
	if (ctr_cfg->cpu_id != arch_cpu_id()) {
		struct counter *ctr;

		/* cross-core counter: read cached value only */
		ctr = ctr_cfg->counter;
		current = ctr->current;
	} else
#endif
	{
		current = counter_query(ctr_cfg);
	}

	elapsed = current - previous;

	SET_OUT1(current);
	SET_OUT2(elapsed);
	SET_RET(E_OK);
}

/** notification from the board that a hardware counter has changed:
 * - "ctr" refers to the counter set with the "register" (reg) hook
 * - "increment" reflects the increment since the last query operation
 */
void kernel_increment_counter(const struct counter_cfg *ctr_cfg, ctrtick_t increment)
{
	struct counter *ctr;
	struct alarm *alm;
	struct alarm *cyclic;
	ctrtick_t current;

	assert(ctr_cfg != NULL);
	assert(ctr_cfg->cpu_id == arch_cpu_id());
	ctr = ctr_cfg->counter;

	assert(increment <= ctr_cfg->maxallowedvalue);
	current = ctr_add(ctr->current, increment, ctr_cfg->maxallowedvalue);
	ctr->current = current;

	cyclic = NULL;

	/* check for pending expiry */
	while ((alm = ctr->first) != NULL) {
		if (ctr_diff(alm->expiry, current, ctr_cfg->maxallowedvalue) < increment) {
			ctr->first = alm->next;
			/* alarm_expire() clears alm->next */
			alarm_expire(alm);

			/* remember cyclic alarms for re-insertion */
			if (alm->cycle != 0) {
				alm->next = cyclic;
				cyclic = alm;
			}
		} else {
			break;
		}
	}

	/* re-insert cyclic alarms */
	while (cyclic != NULL) {
		alm = cyclic;
		cyclic = alm->next;
#ifndef NDEBUG
		alm->next = NULL;
#endif

		alm->expiry = ctr_add(alm->expiry, alm->cycle, ctr_cfg->maxallowedvalue);
		alarm_enqueue(alm, ctr_cfg);
	}
}

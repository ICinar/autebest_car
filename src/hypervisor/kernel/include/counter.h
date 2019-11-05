/*
 * counter.h
 *
 * OSEK counters
 *
 * azuepke, 2014-06-05: initial
 */

#ifndef __COUNTER_H__
#define __COUNTER_H__

#include <counter_state.h>
#include <hv_types.h>

/** Counter configuration -> config.c */
extern const uint8_t num_counters;
extern const struct counter_cfg counter_cfg[];

/** Counter access configuration -> config.c */
extern const struct counter_access counter_access[];

/** Initializer */
void counter_init_all_per_cpu(void);


/* counter handling */

/** Increment a counter value "val" by "inc", handling wrap around at "max". */
static inline ctrtick_t ctr_add(ctrtick_t val, ctrtick_t inc, ctrtick_t max)
{
	assert(val <= max);
	assert(inc <= max);

	if (inc <= max - val) {	/* val + inc <= max */
		return val + inc;
	} else {
		return val + inc - (max + 1);
	}
}

/** Get relative time between two counter values "val1" and "val2",
 *  with "val1" being in the past relatively to "val2"
 *  and handling a wrap around at "max".
 */
static inline ctrtick_t ctr_diff(ctrtick_t val1, ctrtick_t val2, ctrtick_t max)
{
	assert(val1 <= max);
	assert(val2 <= max);

	if (val1 <= val2) {
		return val2 - val1;
	} else {
		return val2 + (max + 1) - val1;
	}
}

/** query a counter to get the internal current value */
static inline ctrtick_t counter_query(const struct counter_cfg *ctr_cfg)
{
	struct counter *ctr;

	assert(ctr_cfg != NULL);
	assert(ctr_cfg->cpu_id == arch_cpu_id());

	ctr = ctr_cfg->counter;
	if (ctr_cfg->type == COUNTER_TYPE_HW) {
		ctr->current = ctr_cfg->query(ctr_cfg);
	}

	assert(ctr->current <= ctr_cfg->maxallowedvalue);
	return ctr->current;
}

/** notify a hardware counter about a change in the alarms */
static inline void counter_change_hw(const struct counter_cfg *ctr_cfg)
{
	assert(ctr_cfg != NULL);
	assert(ctr_cfg->type == COUNTER_TYPE_HW);

	ctr_cfg->change(ctr_cfg);
}


/** Increment (software) counter by one tick */
__tc_fastcall void sys_ctr_increment(unsigned int ctr_id);
/** Get current counter value in ticks */
__tc_fastcall void sys_ctr_get(unsigned int ctr_id);
/** Get elapsed counter value in ticks */
__tc_fastcall void sys_ctr_elapsed(unsigned int ctr_id, ctrtick_t previous);


#endif

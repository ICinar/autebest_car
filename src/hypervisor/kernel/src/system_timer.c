/*
 * system_timer.c
 *
 * OSEK counter-based system timer
 *
 * azuepke, 2014-09-08: stubbed out from board layer
 * azuepke, 2015-06-09: support a dedicated timer per core
 */

#include <kernel.h>
#include <assert.h>
#include <system_timer.h>
#include <counter.h>
#include <core.h>

#ifdef SMP
#define NUM_COUNTERS MAX_CPUS
#else
#define NUM_COUNTERS 1
#endif

/** register counter, NOTE: this is called on its associated CPU */
void system_timer_register(const struct counter_cfg *ctr_cfg)
{
	unsigned int cpu;

	assert(ctr_cfg != NULL);
	cpu = ctr_cfg->cpu_id;
	assert(cpu == arch_cpu_id());


	assert(core_cfg[cpu].core_state->system_timer_ctr_cfg == NULL);
	core_cfg[cpu].core_state->system_timer_ctr_cfg = ctr_cfg;
}

/** return current system time (in time units of the system timer, e.g. microseconds) */
ctrtick_t system_timer_query(const struct counter_cfg *ctr_cfg __unused)
{
	unsigned int cpu = arch_cpu_id();

	assert(ctr_cfg == core_cfg[cpu].core_state->system_timer_ctr_cfg);
	assert(ctr_cfg->cpu_id == cpu);

	return core_cfg[cpu].core_state->system_timer_count;
}

/** set absolute timer expiry time (in time units of the system timer, e.g. microseconds) */
void system_timer_change(const struct counter_cfg *ctr_cfg __unused)
{
	unsigned int cpu = arch_cpu_id();

	assert(ctr_cfg == core_cfg[cpu].core_state->system_timer_ctr_cfg);
	assert(ctr_cfg->cpu_id == cpu);

	/* not implemented, we receive every interrupt */
	(void)cpu;
}

/** increment system timer counter */
void system_timer_increment(void)
{
	unsigned int cpu = arch_cpu_id();

	assert(core_cfg[cpu].core_state->system_timer_ctr_cfg != NULL);
	assert(core_cfg[cpu].core_state->system_timer_ctr_cfg->cpu_id == cpu);

	core_cfg[cpu].core_state->system_timer_count++;

	/* notify kernel to expiry alarms */
	kernel_increment_counter(core_cfg[cpu].core_state->system_timer_ctr_cfg, 1);
}

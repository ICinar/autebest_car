/*
 * ipi.c
 *
 * Multicore IPI-queue management.
 *
 * azuepke, 2015-03-30: initial
 */

#include <kernel.h>
#include <board.h>
#include <assert.h>
#include <bit.h>
#include <sched.h>
#include <task_state.h>
#include <counter_state.h>
#include <schedtab_state.h>
#include <part_state.h>
#include <ipi_state.h>
#include <ipi.h>
#include <event.h>
#include <hv_error.h>
#include <task.h>
#include <part.h>
#include <wq.h>
#include <hm.h>

/* NOTE: no global init function required -- initially, all counters are zero! */

/** get IPI actions for specific CPUs */
static inline struct ipi_action *ipi_actions(
	unsigned int target_cpu,
	unsigned int source_cpu)
{
	const struct ipi_cfg *cfg;

	assert(target_cpu < num_cpus);
	assert(source_cpu < num_cpus);
	assert(target_cpu != source_cpu);

	cfg = &ipi_cfg[source_cpu];
	return cfg->actions[target_cpu];
}

/** get IPI state for specific CPU */
static inline struct ipi_state *ipi_state(
	unsigned int cpu)
{
	const struct ipi_cfg *cfg;

	assert(cpu < num_cpus);

	cfg = &ipi_cfg[cpu];
	return cfg->state;
}

/** enqueue an IPI action for target CPU */
void ipi_enqueue(unsigned int target_cpu, const void *object, uint8_t action, uint8_t aux)
{
	struct ipi_action *actions;
	struct sched_state *sched;
	struct ipi_state *state;
	unsigned int pos;

	assert(target_cpu < num_cpus);
	assert(target_cpu != arch_cpu_id());
	assert(object != NULL);

	actions = ipi_actions(target_cpu, arch_cpu_id());
	state = ipi_state(arch_cpu_id());

	/* add job */
	pos = state->write_pos[target_cpu];
	actions[pos].action = action;
	actions[pos].aux = aux;
	actions[pos].u.object = object;

	/* update write position */
	pos++;
	if (pos == num_ipi_actions) {
		pos = 0;
	}
	barrier();
#ifndef NDEBUG
	/* must not overflow reader position! */
	{
		struct ipi_state *target_state = ipi_state(target_cpu);
		assert(pos != target_state->read_pos[arch_cpu_id()]);
	}
#endif
	state->write_pos[target_cpu] = pos;

	/* mark IPI pending for target CPU */
	sched = current_sched_state();
	sched->reschedule |= 1U << target_cpu;
}

static inline void ipi_do_action(struct ipi_action *action)
{
	unsigned int err;

	if ((action->action == IPI_ACTION_EVENT) ||
	    (action->action == IPI_ACTION_EVENT_NOERR) ||
	    (action->action == IPI_ACTION_TASK) ||
	    (action->action == IPI_ACTION_HOOK)) {
		/* AB-102: check if target partition is still active */
		unsigned int operating_mode;

		assert(action->u.task != NULL);
		operating_mode = action->u.task->cfg->part_cfg->part->operating_mode;
		if (operating_mode == PART_OPERATING_MODE_IDLE) {
			return;
		}
	}

	switch (action->action) {
	case IPI_ACTION_EVENT_NOERR:
		err = ev_set(action->u.task, 1u << action->aux);
		if (unlikely(err != E_OK)) {
			assert(err == E_OS_STATE);
			/* no error reported here */
		}
		break;

	case IPI_ACTION_EVENT:
		err = ev_set(action->u.task, 1u << action->aux);
		if (unlikely(err != E_OK)) {
			assert(err == E_OS_STATE);
			hm_async_task_error(action->u.task->cfg, HM_ERROR_TASK_STATE_ERROR, 1u << action->aux);
		}
		break;

	case IPI_ACTION_TASK:
		err = task_check_activate(action->u.task);
		if (err == E_OK) {
			task_do_activate(action->u.task);
		} else {
			assert(err == E_OS_LIMIT);
			hm_async_task_error(action->u.task->cfg, HM_ERROR_TASK_ACTIVATION_ERROR, 0);
		}
		break;

	case IPI_ACTION_HOOK:
		err = task_check_activate(action->u.task);
		if (err == E_OK) {
			task_do_activate(action->u.task);
		} else {
			assert(err == E_OS_LIMIT);
			/* no error reported here */
		}
		break;

	case IPI_ACTION_WQ_WAKE:
		/* AB-102: robust on multicore against sudden partition reboots */
		wq_wake(action->u.wq, action->aux);
		break;

	case IPI_ACTION_COUNTER:
		kernel_increment_counter(action->u.counter_cfg, 1);
		break;

	case IPI_ACTION_PART_STATE:
		part_delayed_state_change(action->u.part, action->aux);
		break;

	case IPI_ACTION_SCHEDULE_CHANGE:
		schedule_change(action->u.next_tpschedule);
		break;

	default:
		assert(0);
		break;
	}
}

/** incoming IPI interrupt for target CPU (current one) from source CPU */
void kernel_ipi_handle(unsigned int target_cpu, unsigned int source_cpu)
{
	struct ipi_state *target_state;
	struct ipi_state *source_state;
	struct ipi_action *actions;
	unsigned int pos;

	assert(target_cpu == arch_cpu_id());
	assert(source_cpu < num_cpus);
	assert(target_cpu != source_cpu);

	target_state = ipi_state(target_cpu);
	source_state = ipi_state(source_cpu);
	actions = ipi_actions(target_cpu, source_cpu);

	pos = target_state->read_pos[source_cpu];
	while ((volatile unsigned int)source_state->write_pos[target_cpu] != pos) {
		ipi_do_action(&actions[pos]);

		/* update read position */
		pos++;
		if (pos == num_ipi_actions) {
			pos = 0;
		}
		barrier();
		target_state->read_pos[source_cpu] = pos;
	}
}

/** check if source CPU has pending IPIs for target CPU */
static inline int ipi_pending(unsigned int target_cpu, unsigned int source_cpu)
{
	struct ipi_state *target_state;
	struct ipi_state *source_state;

	target_state = ipi_state(target_cpu);
	source_state = ipi_state(source_cpu);

	return source_state->write_pos[target_cpu] != target_state->read_pos[source_cpu];
}

/** broadcast IPIs to given set of CPUs, excluding caller */
void ipi_send(uint32_t cpu_mask)
{
	unsigned int check_cpu;
	unsigned int cpu;
	unsigned int me;

	assert((cpu_mask & (1U << arch_cpu_id())) == 0);
	assert(cpu_mask != 0);

	board_ipi_broadcast(cpu_mask);

	me = check_cpu = arch_cpu_id();

	/* iterate on all set CPUs in given mask, starting at LSB */
	do {
		cpu = __bit_fls(cpu_mask);
		assert(cpu < num_cpus);

		/* work done on target CPU? */
		if (!ipi_pending(cpu, me)) {
			cpu_mask &= ~(1U << cpu);
		}

		/* in the mean time, did IPIs come in for me? */
		if (ipi_pending(me, check_cpu)) {
			kernel_ipi_handle(me, check_cpu);
		}

		check_cpu++;
		if (check_cpu == num_cpus) {
			check_cpu = 0;
		}

		/* Yield CPU */
		arch_yield();
	} while (cpu_mask != 0);
}

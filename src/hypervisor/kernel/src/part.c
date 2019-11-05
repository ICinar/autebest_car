/*
 * part.c
 *
 * Kernel partition handling.
 *
 * azuepke, 2013-11-25: initial
 */

#include <kernel.h>
#include <assert.h>
#include <part.h>
#include <task.h>
#include <sched.h>
#include <hv_error.h>
#include <alarm.h>
#include <schedtab.h>
#include <wq.h>
#include <board.h>
#include <ipi.h>
#include <rpc.h>


/* forward */
static void part_set_operating_mode_self(unsigned int new_mode);

/** start a partition (make all runnable tasks runnable) */
static void part_start(struct part *part, unsigned int new_mode);

/** shutdown a partition (terminate all tasks) */
static void part_shutdown(struct part *part);



/** initialize idle partitions:
 * - initializes the idle partitions in the system and performs various checks
 * - the first partitions are idle partitions (one for each CPU)
 *
 * NOTE: called ONCE during system startup
 */
__init void part_init_idle(void)
{
	const struct part_cfg *part_cfg;
	struct part *part;
	unsigned int i;

	assert(num_partitions >= num_cpus);

	/* initialize dynamic part of idle partitions */
	for (i = 0; i < num_cpus; i++) {
		part_cfg = part_get_part_cfg(i);
		part = part_cfg->part;

		part->cfg = part_cfg;

		/* idle partition */
		assert(part_cfg->initial_operating_mode == PART_OPERATING_MODE_NORMAL);
		assert(!(part_cfg->flags & PART_FLAG_RESTARTABLE));
		assert(part_cfg->num_tasks == 1);

		part->operating_mode = PART_OPERATING_MODE_IDLE;
		part->warm_startable = 0;
		part->start_condition = 0;

		assert(part_cfg->user_sched_state != NULL);

		assert(part_cfg->mpu_part_cfg != NULL);
	}
}

/** initialize all remaining partitions:
 * - iterates all partitions in the system and performs various checks
 * - skips the first partitions are idle partitions (one for each CPU)
 * - after this call, the partitions are properly initialized
 * - all partitions will be in PART_OPERATING_MODE_IDLE afterwards
 *
 * NOTE: called ONCE during system startup
 */
__init void part_init_rest(unsigned int start_condition)
{
	const struct part_cfg *part_cfg;
	struct part *part;
	unsigned int i;

	VVprintf("* partition init: %d partitions\n", num_partitions);

	assert((start_condition == PART_START_CONDITION_NORMAL_START) ||
	       (start_condition == PART_START_CONDITION_HM_MODULE_RESTART));

	/* dump status of idle partitions as well */
	for (i = 0; i < num_cpus; i++) {
		part_cfg = part_get_part_cfg(i);

		Vprintf("  * partition %d '%s': %d tasks\n", i, part_cfg->name,
	            part_cfg->num_tasks);
	}

	/* initialize dynamic part of partitions */
	for (i = num_cpus; i < num_partitions; i++) {
		part_cfg = part_get_part_cfg(i);
		part = part_cfg->part;

		Vprintf("  * partition %d '%s': %d tasks\n", i, part_cfg->name,
	            part_cfg->num_tasks);

		part->cfg = part_cfg;

		/* normal partition */
		assert((part_cfg->initial_operating_mode == PART_OPERATING_MODE_IDLE) ||
		       (part_cfg->initial_operating_mode == PART_OPERATING_MODE_COLD_START));
		assert(part_cfg->num_tasks > 0);

		part->operating_mode = PART_OPERATING_MODE_IDLE;
		part->warm_startable = 0;
		part->start_condition = start_condition;

		assert(part_cfg->user_sched_state != NULL);

		assert(part_cfg->mpu_part_cfg != NULL);

		assert(part_cfg->period > 0);
		assert(part_cfg->duration > 0 && part_cfg->duration <= part_cfg->period);
	}
}

/** startup all partitions (which want to be started)
 * - starts all partitions that not want to be kept in idle state
 * - skips idle partitions (initialized by sched_start())
 *
 * NOTE: called ONCE during system startup
 */
__init void part_start_all(unsigned int cpu_id __unused)
{
	const struct part_cfg *part_cfg;
	struct part *part;
	unsigned int i;
	int start;

	for (i = num_cpus; i < num_partitions; i++) {
		part_cfg = part_get_part_cfg(i);

#ifdef SMP
		if (part_cfg->cpu_id != cpu_id) {
			continue;
		}
#endif

		part = part_cfg->part;

		start = (part_cfg->initial_operating_mode != PART_OPERATING_MODE_IDLE);
		Vprintf("* partition %d '%s': %s in timepart %d on core %d ...\n",
		        i, part_cfg->name, start ? "starting" : "idle",
		        part_cfg->tp_id, cpu_id);
		if (start) {
			part_start(part, part_cfg->initial_operating_mode);
		}
	}
}

/** start a partition (make all runnable tasks runnable) */
static void part_start(struct part *part, unsigned int new_mode)
{
	const struct part_cfg *part_cfg;
	const struct task_cfg *cfg;
	struct task *task;
	unsigned int i;

	assert(part != NULL);
	part_cfg = part->cfg;
	assert(part_cfg->cpu_id == arch_cpu_id());
	assert(part_cfg != NULL);

	assert(new_mode != PART_OPERATING_MODE_IDLE);
	assert(part->operating_mode == PART_OPERATING_MODE_IDLE);
	part->operating_mode = new_mode;
	/* set first partition activation into far future */
	part->error_write_pos = 0;
	part->pending_mode_change = 0;
	part->next_pending_mode_change = NULL;
	part->last_real_task = NULL;

	for (i = 0; i < part_cfg->num_tasks; i++) {
		task = &part_cfg->tasks[i];
		assert(task != NULL);

		/* notify board to unmask the interrupt source as the ISR task is now ready */
		cfg = task->cfg;
		if (TASK_TYPE_IS_ISR(cfg->cfgflags_type)) {
			assert(cfg->cfgflags_type & TASK_CFGFLAG_ACTIVATABLE);
			if (cfg->cfgflags_type & TASK_CFGFLAG_ISR_UNMASK) {
				board_irq_enable(cfg->irq);
			}
		}
	}

	/* activate partition init hook */
	assert(part_cfg->init_hook_id < part_cfg->num_tasks);
	task = &part_cfg->tasks[part_cfg->init_hook_id];
	assert(task != NULL);
	assert(TASK_STATE_IS_SUSPENDED(task->flags_state));

	/* activate hook */
	task_prepare(task);
	sched_readyq_insert_tail(task);

	/* the init hook never has a deadline */
	assert(task->cfg->capacity <= 0);

	/* NOTE: OSEK autostarts are done by the init hook in user space */
}

/** shutdown a partition (kill all tasks) */
static void part_shutdown(struct part *part)
{
	const struct part_cfg *part_cfg;
	const struct task_cfg *cfg;
	struct schedtab *schedtab;
	struct task *task;
	struct alarm *alm;
	unsigned int i;

	assert(part != NULL);
	part_cfg = part->cfg;
	assert(part_cfg != NULL);

	assert(part->operating_mode != PART_OPERATING_MODE_IDLE);
	part->operating_mode = PART_OPERATING_MODE_IDLE;

	/* terminate all tasks, ISR, hooks ... */
	for (i = 0; i < part_cfg->num_tasks; i++) {
		task = &part_cfg->tasks[i];
		assert(task != NULL);

		/* disable further interrupts */
		cfg = task->cfg;
		if (TASK_TYPE_IS_ISR(cfg->cfgflags_type)) {
			board_irq_disable(cfg->irq);
		}

		/* abort all queued RPCs */
		if (cfg->rpc != NULL) {
			rpc_abort(task, cfg->rpc);
		}

		task_terminate(task, 1);
	}

	/* ... and cancel alarms ... */
	for (i = 0; i < part_cfg->num_alarms; i++) {
		alm = &part_cfg->alarms[i];
		if (alm->state != ALARM_STATE_IDLE) {
			alarm_cancel(alm);
		}
	}

	/* ... and stop schedule tables */
	for (i = 0; i < part_cfg->num_schedtabs; i++) {
		schedtab = &part_cfg->schedtabs[i];
		if (schedtab->state != SCHEDTAB_STATE_STOPPED) {
			schedtab_stop(schedtab);
		}
	}

	/* close all wait queues */
	for (i = 0; i < part_cfg->num_wqs; i++) {
		wq_close(&part_cfg->wqs[i]);
	}
}

/** system call to get the caller's partition operating mode */
void sys_part_get_operating_mode(void)
{
	struct part *part;

	part = current_part_cfg()->part;
	assert(part != NULL);

	SET_RET(part->operating_mode);
}

/** system call to get the caller's partition start condition */
void sys_part_get_start_condition(void)
{
	struct part *part;

	part = current_part_cfg()->part;
	assert(part != NULL);

	SET_RET(part->start_condition);
}

/** system call to change the caller's partitions operating mode */
void sys_part_set_operating_mode(unsigned int new_mode)
{
	if ((new_mode != PART_OPERATING_MODE_IDLE) &&
	    (new_mode != PART_OPERATING_MODE_COLD_START) &&
	    (new_mode != PART_OPERATING_MODE_WARM_START) &&
	    (new_mode != PART_OPERATING_MODE_NORMAL)) {
		SET_RET(E_OS_VALUE);	/* ERRNO: INVALID MODE */
		return;
	}

	part_set_operating_mode_self(new_mode);
	/* error code set by part_set_operating_mode_self() */
}

/** Refresh delayed activations for all tasks waiting to enter NORMAL mode */
static void part_handle_delayed_actications(struct part *part)
{
	const struct part_cfg *part_cfg;
	time_t expiry_time;
	struct task *task;
	unsigned int i;
	time_t now;

	assert(part != NULL);
	assert(part->operating_mode == PART_OPERATING_MODE_NORMAL);

	now = board_get_time();

	part_cfg = part->cfg;
	assert(part_cfg != NULL);
	assert(part_cfg->cpu_id == arch_cpu_id());

	for (i = 0; i < part_cfg->num_tasks; i++) {
		task = &part_cfg->tasks[i];
		assert(task != NULL);

		/* skip non-waiting tasks */
		if (!TASK_STATE_IS_WAIT_ACT(task->flags_state) || (task->expiry_time < FAR_FUTURE)) {
			continue;
		}

		/* adjust expiry time */
		expiry_time = task->expiry_time;
		expiry_time -= FAR_FUTURE;

		list_del(&task->ready_and_timeoutq);

		if (expiry_time == 0) {
			/* start immediately -- must be aperiodic task */
			assert(task->cfg->period <= 0);
			task->last_activation = now;
			sched_readyq_insert_tail(task);

			if (task->cfg->capacity > 0) {
				sched_deadline_start(board_get_time(), task);
			}
		} else {
			/* start later */
			if (task->cfg->period > 0) {
				/* periodic tasks start at the partition's first expiry point */
				assert(task->cfg->timepart->last_release_point > 0);
				expiry_time += task->cfg->timepart->last_release_point + part_cfg->period;
			} else {
				/* aperiodic tasks start now */
				expiry_time += now;
			}

			task->expiry_time = expiry_time;
			task->last_activation = expiry_time;

			/* re-insert into timeout queue */
			list_node_init(&task->ready_and_timeoutq);
			#define ITER list_entry(__ITER__, struct task, ready_and_timeoutq)
			list_add_sorted(&task->cfg->timepart->timeoutq, &task->ready_and_timeoutq, ITER->expiry_time >= expiry_time);
			#undef ITER

			/* deadlines are set when the task becomes active */
		}
	}
}


/**
 * change operating mode of the caller's partition:
 *
 * The calling partition cannot be in IDLE mode, because tasks are running.
 *
 * Valid transitions:           Effect:
 *
 * COLD_START -> IDLE           shutdown
 * COLD_START -> COLD_START     restart
 * COLD_START -> WARM_START     forbidden transition -> error
 * COLD_START -> NORMAL         terminate caller
 *
 * WARM_START -> IDLE           shutdown
 * WARM_START -> COLD_START     restart
 * WARM_START -> WARM_START     restart
 * WARM_START -> NORMAL         terminate caller
 *
 * NORMAL     -> IDLE           shutdown
 * NORMAL     -> COLD_START     restart
 * NORMAL     -> WARM_START     restart
 * NORMAL     -> NORMAL         no effect
 */
static void part_set_operating_mode_self(unsigned int new_mode)
{
	struct part *part;

	assert(new_mode <= PART_OPERATING_MODE_NORMAL);

	part = current_part_cfg()->part;
	assert(part != NULL);

	/* we can't be idle */
	assert(part->operating_mode != PART_OPERATING_MODE_IDLE);
	if ((part->operating_mode == PART_OPERATING_MODE_COLD_START) &&
	    (new_mode == PART_OPERATING_MODE_WARM_START)) {
		SET_RET(E_OS_STATE);	/* ERRNO: invalid transition */
		return;
	}

	if (new_mode == PART_OPERATING_MODE_NORMAL) {
		if (part->operating_mode == PART_OPERATING_MODE_NORMAL) {
			SET_RET(E_OS_NOFUNC);	/* ERRNO: no action */
			return;
		}
		assert((part->operating_mode == PART_OPERATING_MODE_COLD_START) ||
		       (part->operating_mode == PART_OPERATING_MODE_WARM_START));

		/* set new mode */
		part->operating_mode = PART_OPERATING_MODE_NORMAL;
		part->warm_startable = 1;

		SET_RET(E_OK);

		/* wake aperiodic tasks waiting to enter NORMAL MODE */
		part_handle_delayed_actications(part);

		return;
	}

	part_delayed_state_change(part, new_mode);
	/* return without setting a return code */
}

/** system call to get other partition's operating mode */
void sys_part_get_operating_mode_ex(unsigned int part_id)
{
	unsigned int part_limit;
	struct part *part;

	if (!(current_part_cfg()->flags & PART_FLAG_PRIVILEGED)) {
		SET_RET(E_OS_ACCESS);	/* ERRNO: partition privilege error */
		return;
	}

	/* skip idle partitions, these are invisible to the user */
	assert(num_partitions >= num_cpus);
	part_limit = num_partitions - num_cpus;
	if (part_id >= part_limit) {
		SET_RET(E_OS_ID);
		return;
	}
	part_id += num_cpus;

	part = part_get_part_cfg(part_id)->part;
	assert(part != NULL);

	SET_OUT1(part->operating_mode);
	SET_OUT2(part->start_condition);
	SET_RET(E_OK);
}

/** system call to change other partition's operating mode
 *
 * If the caller's partition is affected, see part_set_operating_mode_self().
 *
 * Otherwise, the following transition are valid:
 *
 * Valid transitions:           Effect:
 *
 * IDLE       -> IDLE           no effect
 * IDLE       -> COLD_START     restart
 * IDLE       -> WARM_START     forbidden IFF previous mode was COLD_START or IDLE
 * IDLE       -> NORMAL         forbidden transition -> error
 *
 * COLD_START -> IDLE           shutdown
 * COLD_START -> COLD_START     restart
 * COLD_START -> WARM_START     forbidden transition -> error
 * COLD_START -> NORMAL         forbidden transition -> error
 *
 * WARM_START -> IDLE           shutdown
 * WARM_START -> COLD_START     restart
 * WARM_START -> WARM_START     restart
 * WARM_START -> NORMAL         forbidden transition -> error
 *
 * NORMAL     -> IDLE           shutdown
 * NORMAL     -> COLD_START     restart
 * NORMAL     -> WARM_START     restart
 * NORMAL     -> NORMAL         forbidden transition -> error
 *
 * Basically, we cannot set another partition to NORMAL mode,
 * just enforce partition restarts.
 */
void sys_part_set_operating_mode_ex(
	unsigned int part_id,
	unsigned int new_mode)
{
	const struct part_cfg *part_cfg;
	unsigned int part_limit;
	struct part *part;

	if (!(current_part_cfg()->flags & PART_FLAG_PRIVILEGED)) {
		SET_RET(E_OS_ACCESS);	/* ERRNO: partition privilege error */
		return;
	}

	if ((new_mode != PART_OPERATING_MODE_IDLE) &&
	    (new_mode != PART_OPERATING_MODE_COLD_START) &&
	    (new_mode != PART_OPERATING_MODE_WARM_START) &&
	    (new_mode != PART_OPERATING_MODE_NORMAL)) {
		SET_RET(E_OS_VALUE);	/* ERRNO: INVALID MODE */
		return;
	}

	/* skip idle partitions, these are invisible to the user */
	assert(num_partitions >= num_cpus);
	part_limit = num_partitions - num_cpus;
	if (part_id >= part_limit) {
		SET_RET(E_OS_ID);
		return;
	}
	part_id += num_cpus;

	part_cfg = part_get_part_cfg(part_id);
	part = part_cfg->part;
	assert(part != NULL);

	if (!(part_cfg->flags & PART_FLAG_RESTARTABLE)) {
		SET_RET(E_OS_LIMIT);	/* ERRNO: idle partitions cannot be restarted */
		return;
	}

	if (part_cfg == current_part_cfg()) {
		part_set_operating_mode_self(new_mode);
		/* error code set by part_set_operating_mode_self() */
		return;
	}

	if (new_mode == PART_OPERATING_MODE_NORMAL) {
		SET_RET(E_OS_STATE);	/* ERRNO: invalid transition */
		return;
	}

	if (new_mode == PART_OPERATING_MODE_WARM_START) {
		/* additional error checks for WARM_START */
		if (!part->warm_startable) {
			SET_RET(E_OS_STATE);	/* ERRNO: cannot do warm start */
			return;
		}
	}

	SET_RET(E_OK);

#ifdef SMP
	if (part_cfg->cpu_id != arch_cpu_id()) {
		ipi_enqueue(part_cfg->cpu_id, part, IPI_ACTION_PART_STATE, new_mode);
		return;
	}
#endif

	part_delayed_state_change(part, new_mode);
}

/** trigger a change of the partition state (delayed until scheduling) */
void part_delayed_state_change(
	struct part *part,
	unsigned int new_mode)
{
	assert(part != NULL);
	assert(part->cfg->cpu_id == arch_cpu_id());

	if (part->pending_mode_change == 0) {
		part->pending_mode_change = 1;
		part->new_operating_mode = new_mode;

		sched_enqueue_part_state_change(part);
	}
}

/** do a change of the partition state (called from scheduler) */
void part_state_change(
	struct part *part)
{
	unsigned int new_mode;

	assert(part != NULL);
	assert(part->cfg->cpu_id == arch_cpu_id());

	assert(part->pending_mode_change != 0);
	part->pending_mode_change = 0;

	new_mode = part->new_operating_mode;
	assert((new_mode == PART_OPERATING_MODE_IDLE) ||
	       (new_mode == PART_OPERATING_MODE_COLD_START) ||
	       (new_mode == PART_OPERATING_MODE_WARM_START));

	/* request for warm start, but not capable */
	if ((new_mode == PART_OPERATING_MODE_WARM_START) && !part->warm_startable) {
		new_mode = PART_OPERATING_MODE_COLD_START;
	}

	if (new_mode == PART_OPERATING_MODE_COLD_START) {
		/* a transition via COLD_START clears the ability to WARM_START */
		part->warm_startable = 0;
	}

	/* shut down partition, if necessary */
	if (part->operating_mode != PART_OPERATING_MODE_IDLE) {
		part_shutdown(part);
	}
	assert(part->operating_mode == PART_OPERATING_MODE_IDLE);
	part->start_condition = PART_START_CONDITION_PARTITION_RESTART;

	/* ... and probably restart */
	if ((new_mode == PART_OPERATING_MODE_COLD_START) ||
	    (new_mode == PART_OPERATING_MODE_WARM_START)) {
		part_start(part, new_mode);
		assert(part->operating_mode != PART_OPERATING_MODE_IDLE);
	}
}



/** system call to get a partition's partition ID (privileged) */
void sys_part_self(void)
{
	const struct part_cfg *part_cfg;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);

	/* remove idle partitions, these are invisible to the user */
	SET_RET(part_cfg->part_id - num_cpus);
}

/** Check if current partition has access to user space address. */
unsigned int kernel_check_user_addr(
	void *user_addr,
	size_t size)
{
	const struct part_cfg *part_cfg;
	const struct mem_range *range;
	unsigned int i;

	/* check memory range */
	part_cfg = current_part_cfg();
	for (i = 0; i < NUM_MEM_RANGES; i++) {
		range = &part_cfg->mem_ranges[i];
		if ((range->start <= (addr_t)user_addr) &&
		    ((addr_t)user_addr + size < range->end)) {
			/* match */
			return E_OK;
		}
	}

	/* no range matched */
	return E_OS_ILLEGAL_ADDRESS;
}

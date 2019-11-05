/*
 * task.c
 *
 * Kernel task handling.
 *
 * azuepke, 2013-11-25: initial
 */

#include <kernel.h>
#include <assert.h>
#include <task.h>
#include <part.h>
#include <sched.h>
#include <hv_error.h>
#include <board.h>
#include <rpc.h>


/* forward declarations */
static void task_pending_activations(struct task *task);

/** initialize idle tasks only
 *
 * NOTE: called ONCE during system startup
 */
__init void task_init_idle(void)
{
	struct arch_ctxt_frame *ctxt;
	const struct task_cfg *cfg;
	struct arch_reg_frame *regs;
	struct task *task;
	unsigned int i;

	assert(num_tasks > 0);
	assert(num_tasks >= num_cpus);

	/* initialize dynamic part of the idle tasks on each CPU core */
	for (i = 0; i < num_cpus; i++) {
		cfg = task_get_task_cfg(i);
		task = cfg->task;

		assert(cfg->part_cfg == part_get_part_cfg(i));
		assert(cfg->task_id == 0);
		task->cfg = cfg;

		assert(cfg->max_activations == 1);

		regs = cfg->regs;
		assert(regs != NULL);
		arch_reg_frame_init(regs);

		/* idle tasks have no FPU frame */
		assert(cfg->fpu == NULL);

		ctxt = cfg->ctxt;
		assert(ctxt != NULL);
		arch_ctxt_frame_init(regs, ctxt, cfg->num_ctxts);

		assert(cfg->mpu_task_cfg != NULL);

		task->flags_state = TASK_COPY_FLAGS(cfg->cfgflags_type);

		/* no deadline */
		assert(cfg->capacity <= 0);
	}
}

/** initialize all tasks
 * - iterates all tasks in the system and performs various checks
 * - after this call, the task tables are properly initialized
 * - all tasks will be in TASK_STATE_SUSPENDED afterwards
 *
 * NOTE: called ONCE during system startup
 */
__init void task_init_rest(void)
{
	struct arch_ctxt_frame *ctxt;
	const struct task_cfg *cfg;
	struct arch_reg_frame *regs;
	struct task *task;
	unsigned int i;

	VVprintf("* task init: %d tasks\n", num_tasks);

#ifndef NDEBUG
	/* just print the idle tasks' status */
	for (i = 0; i < num_cpus; i++) {
		cfg = task_get_task_cfg(i);
		Vprintf("  * %s %d '%s'", "idle ", i, cfg->name);
		Vprintf(" regs: 0x%p", cfg->regs);
		Vprintf(" ctxt: 0x%p (%d)", cfg->ctxt, cfg->num_ctxts);
		Vprintf("\n");
	}
#endif

	/* initialize dynamic part of the remaining tasks (except idle tasks) */
	for (i = num_cpus; i < num_tasks; i++) {
		cfg = task_get_task_cfg(i);
		task = cfg->task;

		Vprintf("  * %s %d '%s'", TASK_TYPE_IS_ISR(cfg->cfgflags_type) ? "ISR  " :
		                          TASK_TYPE_IS_HOOK(cfg->cfgflags_type) ? "hook " :
		                          TASK_TYPE_IS_INVOKABLE(cfg->cfgflags_type) ? "invok" :
		                          TASK_MAY_BLOCK(cfg->cfgflags_type) ? "etask" : "btask",
		                          i, cfg->name);

		task->cfg = cfg;

		assert(cfg->max_activations >= 1);
#ifndef NDEBUG
		if (task->cfg->task_id == cfg->part_cfg->error_hook_id) {
			assert(cfg->max_activations == cfg->part_cfg->num_error_states);
		}
#endif
#if MAX_TASK_ACTIVATIONS != 255
		assert(cfg->max_activations <= MAX_TASK_ACTIVATIONS);
#endif

		regs = cfg->regs;
		arch_reg_frame_init(regs);
		Vprintf(" regs: 0x%p", cfg->regs);

		if (cfg->fpu != NULL) {
			/* allocate an FPU context */
			arch_fpu_frame_init(cfg->fpu);
			Vprintf(" FPU: 0x%p", cfg->fpu);
		}

		ctxt = cfg->ctxt;
		assert(ctxt != NULL);
		arch_ctxt_frame_init(regs, ctxt, cfg->num_ctxts);
		Vprintf(" ctxt: 0x%p (%d)", cfg->ctxt, cfg->num_ctxts);

		assert(cfg->mpu_task_cfg != NULL);

		if (cfg->rpc != NULL) {
			rpc_init(cfg->rpc);
		}

		Vprintf("\n");
		task->flags_state = TASK_COPY_FLAGS(cfg->cfgflags_type);
	}
}

/** internal preparation for activation of a task */
void task_prepare(struct task *task)
{
	struct arch_reg_frame *regs;
	const struct task_cfg *cfg;

	assert(task != NULL);
	assert(TASK_STATE_IS_SUSPENDED(task->flags_state));

	cfg = task->cfg;
	assert(cfg != NULL);
	assert(cfg->cpu_id == arch_cpu_id());

	/* the elevated priority is taken when the task is scheduled */
	task->task_prio = cfg->base_prio;
	/* state is set by the readyq function */
	task->pending_activations = 0;

	/* reset task flags */
	task->flags_state = TASK_COPY_FLAGS(cfg->cfgflags_type);

	/* events are cleared on activation */
	task->ev_pending = 0;

	/* set deadline */
	if (cfg->capacity > 0) {
#ifndef NDEBUG
		task->deadline = INFINITY;
#endif
		/* NOTE: we init the node as head to allow safe list deletion */
		list_head_init(&task->deadlineq);
	}

	/* set registers */
	regs = cfg->regs;
	arch_reg_frame_assign(regs, cfg->entry, cfg->stack, cfg->stack_size, cfg->arg0);
	arch_reg_frame_init_sda(regs, cfg->part_cfg->sda1_base, cfg->part_cfg->sda2_base);
}

/** check if a task activation will succeed or fail */
unsigned int task_check_activate(struct task *task)
{
	const struct task_cfg *cfg;

	assert(task != NULL);
	assert(task->cfg->cpu_id == arch_cpu_id());
	assert(TASK_TYPE_IS_TASK(task->cfg->cfgflags_type) || TASK_TYPE_IS_HOOK(task->cfg->cfgflags_type));

	if (!TASK_STATE_IS_SUSPENDED(task->flags_state)) {
		/* task is not suspended, check for multiple activation requests */
		cfg = task->cfg;
		if (task->pending_activations < cfg->max_activations - 1) {
			return E_OK;
		}

		return E_OS_LIMIT;
	}

	return E_OK;
}

/** really activate a task or hook */
void task_do_activate(struct task *task)
{
	assert(task != NULL);

	if (TASK_STATE_IS_SUSPENDED(task->flags_state)) {
		/* activate */
		task_prepare(task);
		sched_readyq_insert_tail(task);

		if (task->cfg->capacity > 0) {
			sched_deadline_start(board_get_time(), task);
		}
	} else {
		assert(task->pending_activations < task->cfg->max_activations - 1);
		task->pending_activations++;
	}
}

/** Activate a task */
void sys_task_activate(unsigned int task_id)
{
	const struct part_cfg *part_cfg;
	const struct task_cfg *cfg;
	struct task *task;
	unsigned int err;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);
	if (task_id >= part_cfg->num_tasks) {
		SET_RET(E_OS_ID);
		return;
	}

	task = &part_cfg->tasks[task_id];
	assert(task != NULL);
	cfg = task->cfg;
	if (!TASK_TYPE_IS_TASK(cfg->cfgflags_type)) {
		SET_RET(E_OS_ACCESS);
		return;
	}
	if (!(cfg->cfgflags_type & TASK_CFGFLAG_ACTIVATABLE)) {
		SET_RET(E_OS_ACCESS);
		return;
	}

	err = task_check_activate(task);
	SET_RET(err);

	if (err == E_OK) {
		/* activate */
		task_do_activate(task);
	}
}

/** Activate a task and let it wait on a timeout */
void sys_task_delayed_activate(unsigned int task_id, int sync __unused, timeout_t delay)
{
	const struct part_cfg *part_cfg;
	const struct task_cfg *cfg;
	time_t expiry_time;
	struct task *task;

	if (delay < 0) {
		SET_RET(E_OS_RESOURCE);	/* ERRNO: bad delay */
		return;
	}

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);
	if (task_id >= part_cfg->num_tasks) {
		SET_RET(E_OS_ID);
		return;
	}

	task = &part_cfg->tasks[task_id];
	assert(task != NULL);
	cfg = task->cfg;
	if (!TASK_TYPE_IS_TASK(cfg->cfgflags_type)) {
		SET_RET(E_OS_ACCESS);
		return;
	}
	if (!(cfg->cfgflags_type & TASK_CFGFLAG_ACTIVATABLE)) {
		SET_RET(E_OS_ACCESS);
		return;
	}

	if (!TASK_STATE_IS_SUSPENDED(task->flags_state)) {
		SET_RET(E_OS_STATE);	/* ERRNO: ESTATE */
		return;
	}

	SET_RET(E_OK);

	task_prepare(task);

	/* deadline activated when task finally starts */

	if (part_cfg->part->operating_mode != PART_OPERATING_MODE_NORMAL) {
		/* corner cases: the partition is still COLD_START or WARM_START mode
		 *
		 * We have consider two kind of "delays into the future":
		 * - aperiodic task: delay until partition enters NORMAL mode
		 * - periodic: delay until partition's first expiry point
		 *
		 * We know neither and encode both variants as offsets to FAR_FUTURE.
		 *
		 * When the partition enters NORMAL mode, all aperiodic tasks will be
		 * made ready. Periodic tasks will be set to the first expiry point.
		 */
		expiry_time = FAR_FUTURE + delay;
	} else {
		/* partition in NORMAL mode */
		if (cfg->period > 0) {
			/* periodic, synchronous to next partition release point */
			assert(cfg->timepart->last_release_point > 0);
			expiry_time = cfg->timepart->last_release_point + part_cfg->period + delay;
		} else if (delay == 0) {
			/* aperiodic task, start now */
			goto start_immediately;
		} else {
			assert(delay > 0);
			/* aperiodic task, start later */
			expiry_time = board_get_time() + delay;
		}
	}
	task->expiry_time = expiry_time;

	/* now let the task sleep wait for activation */
	task->flags_state = TASK_SET_STATE(task->flags_state, TASK_STATE_WAIT_ACT);

	/* add to timeout queue */
	list_node_init(&task->ready_and_timeoutq);
	assert(task->cfg->timepart == current_sched_state()->timepart);
	#define ITER list_entry(__ITER__, struct task, ready_and_timeoutq)
	list_add_sorted(&cfg->timepart->timeoutq, &task->ready_and_timeoutq, ITER->expiry_time >= expiry_time);
	#undef ITER

	return;

start_immediately:
	sched_readyq_insert_tail(task);

	if (cfg->capacity > 0) {
		sched_deadline_start(board_get_time(), task);
	}
}

/** system call to create a task */
void sys_task_create(
	unsigned int task_id,
	unsigned int prio,
	void *entry,
	void *stack,
	void *arg0,
	void *arg1)
{
	const struct part_cfg *part_cfg;
	struct arch_reg_frame *regs;
	const struct task_cfg *cfg;
	struct task *task;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);
	if (task_id >= part_cfg->num_tasks) {
		SET_RET(E_OS_ID);
		return;
	}

	if (prio > part_cfg->max_prio) {
		SET_RET(E_OS_VALUE);	/* ERRNO: priority out of limits */
		return;
	}

	task = &part_cfg->tasks[task_id];
	assert(task != NULL);
	cfg = task->cfg;
	if (!TASK_TYPE_IS_TASK(cfg->cfgflags_type)) {
		SET_RET(E_OS_ACCESS);
		return;
	}
	if (cfg->cfgflags_type & TASK_CFGFLAG_ACTIVATABLE) {
		SET_RET(E_OS_ACCESS);
		return;
	}

	if (!TASK_STATE_IS_SUSPENDED(task->flags_state)) {
		/* task is already created */
		SET_RET(E_OS_STATE);	/* ERRNO: ESTATE */
		return;
	}

	if (!arch_check_user_stack(part_cfg, (addr_t)stack)) {
		/* stack does not belong to partition, required for ARM Cortex-M3/M4 */
		SET_RET(E_OS_ILLEGAL_ADDRESS);
		return;
	}

	SET_RET(E_OK);

	task_prepare(task);

	/* override priority */
	task->task_prio = prio;

	/* override registers */
	regs = cfg->regs;
	arch_reg_frame_assign(regs, (unsigned long)entry, (unsigned long)stack, 0,
	                            (unsigned long)arg0);
	arch_reg_frame_set_arg1(regs, (unsigned long)arg1);
	arch_reg_frame_init_sda(regs, part_cfg->sda1_base, part_cfg->sda2_base);

	/* make READY */
	sched_readyq_insert_tail(task);

	/* deadline cannot be used here */
	assert(cfg->capacity <= 0);
}

/** wake a user ISR task (this is a "task activate" for ISRs) */
/* NOTE: this is called from the board layer */
void kernel_wake_isr_task(const void *arg0)
{
	const struct task_cfg *cfg = arg0;
	struct task *task;

	assert(cfg != NULL);
	assert(cfg->cpu_id == arch_cpu_id());
	task = cfg->task;
	assert(task != NULL);
	assert(TASK_STATE_IS_SUSPENDED(task->flags_state));
	assert(TASK_TYPE_IS_ISR(cfg->cfgflags_type));
	assert(cfg->cfgflags_type & TASK_CFGFLAG_ACTIVATABLE);

	/* mask the associated interrupt source */
	board_irq_disable(cfg->irq);

	/* activate ISR */
	task_prepare(task);
	sched_readyq_insert_tail(task);

	if (cfg->capacity > 0) {
		sched_deadline_start(board_get_time(), task);
	}
}

/** Terminate a task: do cleanup and let it enter TASK_STATE_SUSPENDED state
 * - the task may be in any state
 * NOTE: this is called on partition shutdown as well
 */
void task_terminate(struct task *task, int partition_shutdown)
{
	const struct task_cfg *cfg;

	assert(task != NULL);
	cfg = task->cfg;
	assert(cfg->cpu_id == arch_cpu_id());

	if (!TASK_STATE_IS_SUSPENDED(task->flags_state) && (cfg->capacity > 0)) {
		/* remove from deadline queue */
		list_del(&task->deadlineq);
	}

	switch (TASK_STATE(task->flags_state)) {
	case TASK_STATE_RUNNING:
		/* terminate calling task */
		assert(task == current_task());
		sched_suspend(task);
		break;

	case TASK_STATE_READY:
		/* pull the task from the ready queue */
		sched_readyq_remove(task);
		break;

	case TASK_STATE_WAIT_SEND:
	case TASK_STATE_WAIT_RECV:
		/* if the task is involved in RPC, unlink from RPC */
		rpc_cancel(task);
		/* FALL-THROUGH */

	case TASK_STATE_WAIT_WQ:
		/* task enqueued on a wait_queue, remove */
		list_del(&task->waitq);
		/* FALL-THROUGH */

	case TASK_STATE_WAIT_ACT:
	case TASK_STATE_WAIT_EV:
		/* probably timeout, remove as well */
		list_del(&task->ready_and_timeoutq);
		/* FALL-THROUGH */

	case TASK_STATE_SUSPENDED:
		/* task is not on the ready queue */
		task->flags_state = TASK_SET_STATE(task->flags_state, TASK_STATE_SUSPENDED);
		break;

	/* no default rule here */
	}
	assert(TASK_STATE_IS_SUSPENDED(task->flags_state));

	/* for ISR tasks, notify board to unmask the interrupt source again */
	if (TASK_TYPE_IS_ISR(cfg->cfgflags_type) && !partition_shutdown) {
		board_irq_enable(cfg->irq);
	}
}

/** Terminate any other task (except the current one) */
void sys_task_terminate_other(unsigned int task_id)
{
	const struct part_cfg *part_cfg;
	struct task *task;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);
	if (task_id >= part_cfg->num_tasks) {
		SET_RET(E_OS_ID);
		return;
	}

	task = &part_cfg->tasks[task_id];
	assert(task != NULL);

	if (task == current_task()) {
		SET_RET(E_OS_STATE);	/* ERRNO: cannotkillself */
		return;
	}
	assert(task != current_task());
	assert(!TASK_STATE_IS_RUNNING(task->flags_state));

	if (TASK_STATE_IS_SUSPENDED(task->flags_state)) {
		SET_RET(E_OS_NOFUNC);	/* ERROR: no effect, already terminated */
		return;
	}

	SET_RET(E_OK);

	task_terminate(task, 0);

	/* activate again if still has pending activations! */
	task_pending_activations(task);
}

/** Terminate the current task */
void task_terminate_self(struct task *task)
{
	const struct task_cfg *cfg;

	assert(task == current_task());
	cfg = task->cfg;

	if (cfg->capacity > 0) {
		/* remove from deadline queue */
		list_del(&task->deadlineq);
	}

	sched_suspend(task);

	/* NOTE: activate again if still has pending activations! */
	task_pending_activations(task);

	/* return without setting a return code */
}

/** Re-activate a task due to pending activations */
static void task_pending_activations(struct task *task)
{
	const struct task_cfg *cfg;

	assert(task != NULL);
	cfg = task->cfg;

	if (task->pending_activations > 0) {
		/* task_prepare() clears all pending activations, so we handle them */
		unsigned int pending_activations;
		pending_activations = task->pending_activations - 1;

		/* activate again */
		task_prepare(task);
		task->pending_activations = pending_activations;
		if (cfg->rpc != NULL) {
			rpc_next(task, cfg->rpc);
		}
		sched_readyq_insert_tail(task);

		if (cfg->capacity > 0) {
			sched_deadline_start(board_get_time(), task);
		}
	}

	if (TASK_TYPE_IS_ISR(cfg->cfgflags_type)) {
		board_irq_enable(cfg->irq);
	}
}

/** Terminate the current task, Syscall */
void sys_task_terminate(void)
{
	task_terminate_self(current_task());
}

/** Terminate the current task and activate another task */
void sys_task_chain(unsigned int task_id)
{
	const struct part_cfg *part_cfg;
	const struct task_cfg *cfg;
	struct task *task;
	unsigned int err;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);
	if (task_id >= part_cfg->num_tasks) {
		SET_RET(E_OS_ID);
		return;
	}

	task = &part_cfg->tasks[task_id];
	assert(task != NULL);
	cfg = task->cfg;
	if (!TASK_TYPE_IS_TASK(cfg->cfgflags_type)) {
		SET_RET(E_OS_ACCESS);
		return;
	}
	if (!(cfg->cfgflags_type & TASK_CFGFLAG_ACTIVATABLE)) {
		SET_RET(E_OS_ACCESS);
		return;
	}

	if (task == current_task()) {
		/* chaining to self does not change the number of pending activations */
		err = E_OK;
	} else {
		err = task_check_activate(task);
	}
	SET_RET(err);

	if (err == E_OK) {
		task_terminate_self(current_task());

		/* activate */
		task_do_activate(task);
	}
}

/**
 *  system call to retrieve the current task's task ID, including ISRs and hooks
 */
// FIXME: syscall not needed, use lookup in __sys_sched_state.taskid instead
void sys_task_self(void)
{
	const struct task_cfg *cfg;
	struct task *task;

	task = current_task();
	cfg = task->cfg;

	SET_RET(cfg->task_id);
}

/**
 *  system call to retrieve the current task's task ID, excluding ISRs and hooks
 *  When an ISR calls this, the ID refers to the latest preempted task.
 */
void sys_task_current(void)
{
	const struct task_cfg *cfg;
	unsigned int task_id;
	struct task *task;

	task = current_part_cfg()->part->last_real_task;
	if (task != NULL) {
		cfg = task->cfg;
		task_id = cfg->task_id;
	} else {
		task_id = -1;	/* ERRNO: INVALID_TASK */
	}

	SET_RET(task_id);
}

/**
 *  system call to retrieve the current ISR's task ID
 *  When a normal task or hook calls this, an invalid ID is returned.
 */
// FIXME: syscall not needed, libos can resolve the ISR ID in user space
void sys_task_isrid(void)
{
	const struct task_cfg *cfg;
	unsigned int isr_id;
	struct task *task;

	task = current_task();
	cfg = task->cfg;
	if (TASK_TYPE_IS_ISR(cfg->cfgflags_type)) {
		isr_id = cfg->task_id;
	} else {
		isr_id = -1;	/* ERRNO: NOTANISR */
	}

	SET_RET(isr_id);
}

/** Set priority of a task (always enqueued at the end of the ready queue) */
void sys_task_set_prio(unsigned int task_id, unsigned int prio)
{
	const struct part_cfg *part_cfg;
	struct task *task;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);
	if (task_id >= part_cfg->num_tasks) {
		SET_RET(E_OS_ID);
		return;
	}

	if (prio > part_cfg->max_prio) {
		SET_RET(E_OS_VALUE);	/* ERRNO: priority out of limits */
		return;
	}

	task = &part_cfg->tasks[task_id];
	assert(task != NULL);
	/* allowed for all task classes */

	SET_RET(E_OK);

	switch (TASK_STATE(task->flags_state)) {
	case TASK_STATE_RUNNING:
		assert(task == current_task());
		current_prio_set(prio);
		return;

	case TASK_STATE_READY:
		sched_readyq_remove(task);
		task->task_prio = prio;
		sched_readyq_insert_tail(task);
		return;

	case TASK_STATE_WAIT_EV:
	case TASK_STATE_WAIT_WQ:
	case TASK_STATE_WAIT_ACT:
	case TASK_STATE_WAIT_SEND:
		/* NOTE: don't update order in wait queues, ARINC doesn't demaid that */
		task->task_prio = prio;
		return;

	default:
		assert(TASK_STATE_IS_SUSPENDED(task->flags_state));
		SET_RET(E_OS_STATE);	/* ERRNO: ESTATE */
		return;
	}
}

/** Get priority of a task */
void sys_task_get_prio(unsigned int task_id)
{
	const struct part_cfg *part_cfg;
	struct task *task;
	unsigned int prio;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);
	if (task_id >= part_cfg->num_tasks) {
		SET_RET(E_OS_ID);
		return;
	}

	task = &part_cfg->tasks[task_id];
	assert(task != NULL);
	/* allowed for all task classes */

	if (task == current_task()) {
		prio = current_prio_get();
	} else {
		prio = task->task_prio;
	}

	SET_OUT1(prio);
	SET_RET(E_OK);
}

/** Get scheduling state of a task */
void sys_task_get_state(unsigned int task_id)
{
	const struct part_cfg *part_cfg;
	struct task *task;
	unsigned int state;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);
	if (task_id >= part_cfg->num_tasks) {
		SET_RET(E_OS_ID);
		return;
	}

	task = &part_cfg->tasks[task_id];
	assert(task != NULL);
	/* allowed for all task classes */

	/* state translation done in user space */
	state = TASK_STATE(task->flags_state);

	SET_OUT1(state);
	SET_RET(E_OK);
}

/** Mask interrupt source of an interrupt service routine. */
void sys_isr_mask(unsigned int isr_id)
{
	const struct part_cfg *part_cfg;
	const struct task_cfg *cfg;
	struct task *task;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);
	if (isr_id >= part_cfg->num_tasks) {
		SET_RET(E_OS_ID);
		return;
	}

	task = &part_cfg->tasks[isr_id];
	assert(task != NULL);
	cfg = task->cfg;
	if (!TASK_TYPE_IS_ISR(cfg->cfgflags_type)) {
		SET_RET(E_OS_ACCESS);
		return;
	}

	if (task == current_task()) {
		/* FIXME: masking the interrupt source from the ISR routine itself
		 * requires some state trickery to prevent the kernel from unmasking
		 * the interrupt source again!
		 */
		SET_RET(E_OS_STATE);	/* ERRNO: cannot mask self while already masked */
		return;
	}

	SET_RET(E_OK);

	board_irq_disable(cfg->irq);
}

/** Unmask interrupt source of an interrupt service routine. */
void sys_isr_unmask(unsigned int isr_id)
{
	const struct part_cfg *part_cfg;
	const struct task_cfg *cfg;
	struct task *task;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);
	if (isr_id >= part_cfg->num_tasks) {
		SET_RET(E_OS_ID);
		return;
	}

	task = &part_cfg->tasks[isr_id];
	assert(task != NULL);
	cfg = task->cfg;
	if (!TASK_TYPE_IS_ISR(cfg->cfgflags_type)) {
		SET_RET(E_OS_ACCESS);
		return;
	}

	if (!TASK_STATE_IS_SUSPENDED(task->flags_state)) {
		SET_RET(E_OS_STATE);	/* ERRNO: cannot unmask IRQ source while running */
		return;
	}

	SET_RET(E_OK);

	board_irq_enable(cfg->irq);
}

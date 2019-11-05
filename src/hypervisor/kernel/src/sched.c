/*
 * sched.c
 *
 * The scheduler.
 *
 * azuepke, 2013-11-25: initial
 * azuepke, 2014-05-03: use array of linked lists
 * azuepke, 2015-04-30: time partitioning
 */

#include <kernel.h>
#include <assert.h>
#include <sched.h>
#include <core.h>
#include <task.h>
#include <arch.h>
#include <part.h>
#include <bit.h>
#include <hv_error.h>
#include <board.h>
#include <mpu.h>
#include <arch_mpu.h>
#include <ipi.h>
#include <tp.h>
#include <system_timer.h>
#include <hm.h>
#include <rpc.h>

/* forward declarations */
static __noinline struct arch_reg_frame *sched_switch(struct sched_state *sched, struct task *next);
static void sched_timeout_expire(time_t now, struct task *task);
static void tp_switch(struct sched_state *sched);
static void sched_do_part_state_changes(struct sched_state *sched);


/** initialize scheduling */
__init void sched_init(void)
{
	struct timepart_state *timepart;
	struct sched_state *sched;
	unsigned int cpu;
	unsigned int tp;
	unsigned int i;

	VVprintf("* sched init: %d CPUs\n", num_cpus);

	assert(num_cpus > 0);
	assert(num_cpus < MAX_CPUS);

	for (cpu = 0; cpu < num_cpus; cpu++) {
		sched = core_cfg[cpu].sched;

		/* we need to immediately have a valid CURRENT task,
		 * so let's pick the idle tasks early.
		 */
#ifdef SMP
		sched->reschedule = 1U << cpu;
#else
		sched->reschedule = 1;
#endif
		sched->regs = task_get_task_cfg(cpu)->regs;
		assert(sched->regs != NULL);
		sched->fpu = NULL;
		arch_set_kern_stack(sched, core_cfg[cpu].kern_stack, kern_stack_size);
		arch_set_nmi_stack(sched, core_cfg[cpu].nmi_stack, nmi_stack_size);
		arch_set_kern_ctxts(sched, core_cfg[cpu].kern_ctxts, kern_num_ctxts);
		arch_set_nmi_ctxts(sched, core_cfg[cpu].nmi_ctxts, nmi_num_ctxts);

		sched->current_task = task_get_task_cfg(cpu)->task;
		sched->current_part_cfg = part_get_part_cfg(cpu);
		assert(sched->current_part_cfg != NULL);
		sched->user_sched_state = sched->current_part_cfg->user_sched_state;
		assert(sched->user_sched_state != NULL);

		sched->idle_task = task_get_task_cfg(cpu)->task;
		sched->pending_part_mode_change = NULL;

		for (tp = 0; tp < num_timeparts; tp++) {
			timepart = &core_cfg[cpu].timeparts[tp];

			timepart->timepart_id = tp;
			timepart->next_prio = 0;
			list_head_init(&timepart->timeoutq);
			list_head_init(&timepart->deadlineq);

			for (i = 0; i < NUM_PRIOS; i++) {
				list_head_init(&timepart->readyq[i]);
			}
			for (i = 0; i < (NUM_PRIOS >> 5); i++) {
				timepart->active_fine[i] = 0;
			}
			timepart->active_coarse = 0;
			timepart->last_release_point = 0;
		}

		sched->timepart = &core_cfg[cpu].timeparts[0];

		/* time partitioning */
		assert(num_tpschedules > 0);
		sched->tpwindow = &tpwindow_cfg[0];
		sched->wrap_tpwindow = &tpwindow_cfg[0];
	}
}

/** set bit in priority tracking bitmap */
static inline __alwaysinline void readyq_set_bit(struct timepart_state *timepart, uint8_t prio)
{
	unsigned int coarse;

	coarse = prio >> 5;

	timepart->active_fine[coarse] |= 1u << (prio & 31);
	timepart->active_coarse |= 1u << coarse;
}

/** clear bit in priority tracking bitmap */
static inline __alwaysinline void readyq_clear_bit(struct timepart_state *timepart, uint8_t prio)
{
	unsigned int coarse;

	coarse = prio >> 5;

	timepart->active_fine[coarse] &= ~(1u << (prio & 31));
	if (timepart->active_fine[coarse] == 0) {
		timepart->active_coarse &= ~(1u << coarse);
	}
}

/** determine highest scheduling priority in priority tracking bitmap */
static inline __alwaysinline unsigned int readyq_highest(struct timepart_state *timepart)
{
	unsigned int coarse, fine;

	if (timepart->active_coarse == 0) {
		return 0;
	}

	assert(timepart->active_coarse != 0);
	coarse = __bit_ffs(timepart->active_coarse);

	assert(timepart->active_fine[coarse] != 0);
	fine = __bit_ffs(timepart->active_fine[coarse]);

	return (coarse << 5) + fine;
}

/** start scheduling (called once at system start on each CPU) */
__init void sched_start(void)
{
	struct sched_state *sched = current_sched_state();
	const struct task_cfg *cfg;
	struct part *part;
	struct task *task;

	assert(sched != NULL);

	VVprintf("* cpu %d (sched: 0x%p) ready for scheduling\n", arch_cpu_id(), sched);

	/* initialize the idle task */
	task = sched->current_task;
	assert(task != NULL);
	assert(task == sched->idle_task);
	assert(task->task_prio == 0);

	cfg = task->cfg;
	assert(cfg->cpu_id == arch_cpu_id());
	assert(cfg->part_cfg == part_get_part_cfg(arch_cpu_id()));
	assert(sched->regs == cfg->regs);

	task->flags_state = TASK_COPY_FLAGS(cfg->cfgflags_type);
	assert(cfg->fpu == NULL);
	assert(TASK_TYPE_IS_HOOK(cfg->cfgflags_type));

	/* the idle task is not kept on the ready queue, set to RUNNING state */
	assert(sched->timepart->timepart_id == 0);
	assert(sched->timepart->next_prio == 0);
	task->flags_state = TASK_SET_STATE(task->flags_state, TASK_STATE_RUNNING);

	/* dito for the according idle partition: set to NORMAL mode */
	part = cfg->part_cfg->part;
	assert(part->operating_mode == PART_OPERATING_MODE_IDLE);
	part->operating_mode = PART_OPERATING_MODE_NORMAL;

	/* start time partitioning (FIXME: not synchronized to other cores!!!) */
	sched->last_tp_switch = board_get_time();
	sched->next_tp_switch = sched->last_tp_switch + sched->tpwindow->duration;

	/* setup register context for return into the idle task */
	arch_reg_frame_assign_idle(sched->regs, (unsigned long) board_idle,
	                             core_cfg[arch_cpu_id()].idle_stack,
	                             idle_stack_size, (unsigned long) sched);

	/* on return, we reschedule and leave to user space (or the idle task) */
}

/** synchronize user priority, returns bounded priority */
static inline unsigned int sched_curr_prio(void)
{
	struct sched_state *sched;
	unsigned int user_prio;

	sched = current_sched_state();
	user_prio = sched->user_sched_state->user_prio;

	/* bound prio to partition limit */
	if (user_prio > sched->current_part_cfg->max_prio) {
		user_prio = sched->current_part_cfg->max_prio;
	}

	return user_prio;
}

/** insert a task at the tail of the ready queue
 *  - the task must be not on the ready queue before
 *  - if the ready queue changes, necessary scheduling is triggered
 */
void sched_readyq_insert_tail(struct task *task)
{
	struct timepart_state *timepart;
	struct sched_state *sched;
	unsigned int prio;

	assert(task != NULL);
	assert(task->cfg->cpu_id == arch_cpu_id());
	assert((TASK_STATE_IS_WAIT_EV(task->flags_state)) ||
	       (TASK_STATE_IS_WAIT_WQ(task->flags_state)) ||
	       (TASK_STATE_IS_WAIT_SEND(task->flags_state)) ||
	       (TASK_STATE_IS_WAIT_RECV(task->flags_state)) ||
	       (TASK_STATE_IS_WAIT_ACT(task->flags_state)) ||
	       (TASK_STATE_IS_SUSPENDED(task->flags_state)));

	task->flags_state = TASK_SET_STATE(task->flags_state, TASK_STATE_READY);
	prio = task->task_prio;
	assert(prio < NUM_PRIOS);

	timepart = task->cfg->timepart;

	list_node_init(&task->ready_and_timeoutq);
	list_add_last(&timepart->readyq[prio], &task->ready_and_timeoutq);
	readyq_set_bit(timepart, prio);

	/* update next_prio */
	if (prio > timepart->next_prio) {
		timepart->next_prio = prio;

		sched = current_sched_state();
		if (timepart == sched->timepart) {
			sched->user_sched_state->next_prio = prio;

			/* higher prio, rescheduing required */
			if (prio > sched_curr_prio()) {
				/* let task enter the scheduler on kernel exit */
#ifdef SMP
				sched->reschedule |= 1U << arch_cpu_id();
#else
				sched->reschedule = 1;
#endif
			}
		}
	}
}

/** insert a task at the head of the ready queue
 *  - only for starting hooks (from SUSPENDED state)!
 *  - the task must be not on the ready queue before
 *  - if the ready queue changes, necessary scheduling is triggered
 *
 * NOTE: this is only used to start the exception hook at highest priority
 *       after an exception was caught!!!
 */
void sched_readyq_insert_head(struct task *task)
{
	struct timepart_state *timepart;
	struct sched_state *sched;
	unsigned int prio;

	assert(task != NULL);
	assert(task->cfg->cpu_id == arch_cpu_id());
	assert((TASK_STATE_IS_WAIT_SEND(task->flags_state)) ||
	       (TASK_STATE_IS_WAIT_RECV(task->flags_state)) ||
	       (TASK_STATE_IS_SUSPENDED(task->flags_state)));

	task->flags_state = TASK_SET_STATE(task->flags_state, TASK_STATE_READY);
	prio = task->task_prio;
	assert(prio < NUM_PRIOS);

	timepart = task->cfg->timepart;

	list_node_init(&task->ready_and_timeoutq);
	list_add_first(&timepart->readyq[prio], &task->ready_and_timeoutq);
	readyq_set_bit(timepart, prio);

	/* update next_prio */
	if (prio >= timepart->next_prio) {
		timepart->next_prio = prio;

		sched = current_sched_state();
		if (timepart == sched->timepart) {
			sched->user_sched_state->next_prio = prio;

			/* let task enter the scheduler on kernel exit */
			assert(timepart == sched->timepart);
#ifdef SMP
			sched->reschedule |= 1U << arch_cpu_id();
#else
			sched->reschedule = 1;
#endif
		}
	}
}

/** insert a running task at the head of the ready queue due preemption
 *  - the task must be not on the ready queue before
 *  - if the ready queue changes, necessary scheduling is triggered
 */
static void sched_readyq_insert_current_head(struct task *task)
{
	struct timepart_state *timepart;
	unsigned int prio;

	assert(task != NULL);
	assert(TASK_STATE_IS_RUNNING(task->flags_state));

	task->flags_state = TASK_SET_STATE(task->flags_state, TASK_STATE_READY);
	prio = sched_curr_prio();
	assert(prio < NUM_PRIOS);
	task->task_prio = prio;

	timepart = task->cfg->timepart;

	list_node_init(&task->ready_and_timeoutq);
	list_add_first(&timepart->readyq[prio], &task->ready_and_timeoutq);
	readyq_set_bit(timepart, prio);

	/* update next_prio */
	if (prio > timepart->next_prio) {
		timepart->next_prio = prio;

		/* we get preempted, no need to update next_prio in user space */
	}
}

/** insert a running task at the head of the ready queue due a yield operation
 *  - the task must be not on the ready queue before
 *  - if the ready queue changes, necessary scheduling is triggered
 */
static void sched_readyq_insert_current_tail(struct task *task)
{
	struct timepart_state *timepart;
	unsigned int prio;

	assert(task != NULL);
	assert(TASK_STATE_IS_RUNNING(task->flags_state));

	task->flags_state = TASK_SET_STATE(task->flags_state, TASK_STATE_READY);
	prio = sched_curr_prio();
	assert(prio < NUM_PRIOS);
	task->task_prio = prio;

	timepart = task->cfg->timepart;

	list_node_init(&task->ready_and_timeoutq);
	list_add_last(&timepart->readyq[prio], &task->ready_and_timeoutq);
	readyq_set_bit(timepart, prio);

	/* update next_prio */
	if (prio > timepart->next_prio) {
		timepart->next_prio = prio;

		/* we get preempted, no need to update next_prio in user space */
	}
}


/** remove a task from the ready queue
 *  - the task must be in ready state and enqueued on the ready queue
 *  - the task is taken from the ready queue and enters SUSPENDED state
 *  - if the ready queue changes, necessary scheduling is triggered
 */
void sched_readyq_remove(struct task *task)
{
	struct timepart_state *timepart;
	struct sched_state *sched;
	unsigned int prio;

	assert(task != NULL);
	assert(task->cfg->cpu_id == arch_cpu_id());
	assert(TASK_STATE_IS_READY(task->flags_state));

	task->flags_state = TASK_SET_STATE(task->flags_state, TASK_STATE_SUSPENDED);
	prio = task->task_prio;
	assert(prio < NUM_PRIOS);

	timepart = task->cfg->timepart;

	list_del(&task->ready_and_timeoutq);
	if (list_is_empty(&timepart->readyq[prio])) {
		readyq_clear_bit(timepart, prio);

		/* removed the last task on current priority level:
		 * per definition, this cannot have an effect on the current runner,
		 * because the current runner must have higher priority
		 */

		/* find new next_prio, if necessary */
		if ((prio == timepart->next_prio) && (prio > 0)) {
			/* find new highest active priority level */
			prio = readyq_highest(timepart);
			timepart->next_prio = prio;

			sched = current_sched_state();
			if (timepart == sched->timepart) {
				sched->user_sched_state->next_prio = prio;
			}
		}
	}
}

/** suspend scheduling for the current task
 *  - the task must be in ready state
 *  - the task must be CURRENT and NOT enqueued on the ready queue
 *  - rescheduling is enforced
 */
void sched_suspend(struct task *task)
{
	struct sched_state *sched = current_sched_state();

	assert(task != NULL);
	assert(task->cfg->cpu_id == arch_cpu_id());
	assert(TASK_STATE_IS_RUNNING(task->flags_state));

	/* enter the scheduler on kernel exit */
#ifdef SMP
	sched->reschedule |= 1U << arch_cpu_id();
#else
	sched->reschedule = 1;
#endif

	task->flags_state = TASK_SET_STATE(task->flags_state, TASK_STATE_SUSPENDED);
	/* task is dead, we don't care about the user space priority anymore */
}

/** let current task wait
 *  - the task must be in ready state
 *  - the task must be CURRENT and NOT enqueued on the ready queue
 *  - rescheduling is enforced
 */
static void sched_wait_internal(struct task *task, unsigned int new_state)
{
	struct sched_state *sched = current_sched_state();
	unsigned int prio;

	assert(task != NULL);
	assert(task->cfg->cpu_id == arch_cpu_id());
	assert(TASK_STATE_IS_RUNNING(task->flags_state));
	assert((new_state == TASK_STATE_WAIT_EV) ||
	       (new_state == TASK_STATE_WAIT_WQ) ||
	       (new_state == TASK_STATE_WAIT_SEND) ||
	       (new_state == TASK_STATE_WAIT_RECV) ||
	       (new_state == TASK_STATE_WAIT_ACT));

	/* enter the scheduler on kernel exit */
#ifdef SMP
	sched->reschedule |= 1U << arch_cpu_id();
#else
	sched->reschedule = 1;
#endif

	task->flags_state = TASK_SET_STATE(task->flags_state, new_state);
	prio = sched_curr_prio();
	assert(prio < NUM_PRIOS);
	task->task_prio = prio;
}

/** let current task wait with timeout */
void sched_wait(struct task *task, unsigned int new_state, timeout_t timeout)
{
	unsigned int adjust;
	time_t expiry_time;

	assert(task != NULL);
	assert(task == current_sched_state()->current_task);
	assert(TASK_STATE_IS_RUNNING(task->flags_state));
	assert((new_state == TASK_STATE_WAIT_EV) ||
	       (new_state == TASK_STATE_WAIT_WQ) ||
	       (new_state == TASK_STATE_WAIT_SEND) ||
	       (new_state == TASK_STATE_WAIT_RECV));

	assert(timeout != 0);

	/* add to timeout queue */
	if (timeout > 0) {
		/* NOTE: we need to adjust the expiry time by the timer's resolution
		 * to guarantee that we have *at least* waited the specified time.
		 */
		adjust = board_timer_resolution - 1;
		expiry_time = board_get_time() + timeout + adjust;
		task->expiry_time = expiry_time;
		list_node_init(&task->ready_and_timeoutq);
		#define ITER list_entry(__ITER__, struct task, ready_and_timeoutq)
		list_add_sorted(&task->cfg->timepart->timeoutq, &task->ready_and_timeoutq, ITER->expiry_time >= expiry_time);
		#undef ITER
	} else {
		/* infinite timeout, never expires, nowhere enqueued */
		assert(timeout <= 0);
#ifndef NDEBUG
		task->expiry_time = INFINITY;
#endif
		/* NOTE: we init the node as head to allow safe list deletion */
		list_head_init(&task->ready_and_timeoutq);
	}

	sched_wait_internal(task, new_state);
}

/** expire the timeout of a waiting task  */
/* NOTE: task must be waiting on the current CPU's timeout queue */
static void sched_timeout_expire(time_t now, struct task *task)
{
	struct arch_reg_frame *regs;
	const struct task_cfg *cfg;

	assert(task != NULL);
	assert(task != current_sched_state()->current_task);
	assert((TASK_STATE_IS_WAIT_WQ(task->flags_state)) ||
	       (TASK_STATE_IS_WAIT_SEND(task->flags_state)) ||
	       (TASK_STATE_IS_WAIT_ACT(task->flags_state)));

	/* remove from timeout queue ... */
	list_del(&task->ready_and_timeoutq);

	cfg = task->cfg;

	if (TASK_STATE_IS_WAIT_ACT(task->flags_state)) {
		/* start deadline of delayed activated tasks */
		/* (wait queue not used here) */
		if (cfg->capacity > 0) {
			sched_deadline_start(now, task);
		}
	} else {
		assert(TASK_STATE_IS_WAIT_WQ(task->flags_state) ||
		       TASK_STATE_IS_WAIT_SEND(task->flags_state));

		if (TASK_STATE_IS_WAIT_SEND(task->flags_state)) {
			rpc_cancel(task);
		}
		/* remove task from wait queue */
		list_del(&task->waitq);

		/* indicate timeout error in registers as well */
		regs = cfg->regs;
		arch_reg_frame_set_return(regs, E_OS_TIMEOUT);
	}

	/* ... and put onto ready queue again */
	sched_readyq_insert_tail(task);
}

/** start the deadline of a task relative to now */
void sched_deadline_start(time_t now, struct task *task)
{
	const struct task_cfg *cfg;
	unsigned int adjust;
	time_t deadline;

	assert(task != NULL);
	cfg = task->cfg;
	assert(cfg != NULL);
	assert(cfg->capacity > 0);

	/* insert on deadline queue */
	/* NOTE: we need to adjust the expiry time by the timer's resolution
	 * to guarantee that we have *at least* waited the specified time.
	 */
	adjust = board_timer_resolution - 1;
	deadline = now + cfg->capacity + adjust;
	assert(task->deadline == INFINITY);
	task->deadline = deadline;

	list_node_init(&task->deadlineq);
	#define ITER list_entry(__ITER__, struct task, deadlineq)
	list_add_sorted(&cfg->timepart->deadlineq, &task->deadlineq, ITER->deadline > deadline);
	#undef ITER
}

/** change the deadline of a task to given expiry time */
void sched_deadline_change(time_t deadline, struct task *task)
{
	const struct task_cfg *cfg;

	assert(task != NULL);
	cfg = task->cfg;
	assert(cfg != NULL);
	assert(cfg->capacity > 0);

	assert(deadline != INFINITY);

	/* re-insert on deadline queue */
	list_del(&task->deadlineq);

	assert(task->deadline != INFINITY);
	task->deadline = deadline;

	list_node_init(&task->deadlineq);
	#define ITER list_entry(__ITER__, struct task, deadlineq)
	list_add_sorted(&cfg->timepart->deadlineq, &task->deadlineq, ITER->deadline > deadline);
	#undef ITER

}

/** disable the deadline of a task  */
void sched_deadline_disable(struct task *task)
{
	assert(task != NULL);
	assert(task->cfg->capacity > 0);

	/* remove from deadline queue */
	list_del(&task->deadlineq);
#ifndef NDEBUG
	task->deadline = INFINITY;
#endif
	/* NOTE: we init the node as head to allow safe list deletion */
	list_head_init(&task->deadlineq);
}

/** remove the first eligible task from a non-empty ready queue */
static inline struct task *readyq_remove_first(struct timepart_state *timepart, unsigned int prio)
{
	struct task *task;
	list_t *node;

	assert(timepart != NULL);
	assert(prio == timepart->next_prio);

	node = __list_first(&timepart->readyq[prio]);
	assert(node != NULL);
	task = list_entry(node, struct task, ready_and_timeoutq);
	assert(TASK_STATE_IS_READY(task->flags_state));
	assert(task->task_prio == prio);
	list_del(&task->ready_and_timeoutq);
	if (list_is_empty(&timepart->readyq[prio])) {
		readyq_clear_bit(timepart, prio);

		/* find new next_prio, if necessary */
		if (prio > 0) {
			/* find new highest active priority level */
			prio = readyq_highest(timepart);
			timepart->next_prio = prio;
			/* user_sched_state is updated in the scheduler */
		}
	}

	return task;
}

/** internal scheduling function */
static struct task *sched_find_next(struct timepart_state *timepart, struct task *idle_task)
{
	struct task *next;
	unsigned int prio;

	if (timepart->active_coarse == 0) {
		/* nothing to schedule, pick idle task */
		next = idle_task;
		assert(TASK_STATE_IS_READY(next->flags_state));
		assert(timepart->next_prio == 0);
		return next;
	}

	prio = timepart->next_prio;
	assert(prio == readyq_highest(timepart));
	next = readyq_remove_first(timepart, prio);
	assert(next != NULL);
	return next;
}

/** the scheduler (called from assembler code) */
/* this routine picks the highest task eligible for scheduling (on this CPU) */
struct arch_reg_frame *sched_schedule(void)
{
	struct sched_state *sched;
	struct task *prev;
	struct task *next;
#ifdef SMP
	uint32_t pending_ipis;
#endif

	sched = current_sched_state();

	assert(sched != NULL);
	assert(sched->current_task != NULL);
	assert(sched->current_part_cfg != NULL);

#ifdef SMP
	pending_ipis = sched->reschedule;
#endif
	sched->reschedule = 0;
#ifdef SMP
	pending_ipis &= ~(1U << arch_cpu_id());
	if (pending_ipis != 0) {
		ipi_send(pending_ipis);
	}
#endif

	/* preempt current task */
	prev = sched->current_task;
	if (TASK_STATE_IS_RUNNING(prev->flags_state)) {
		if (prev == sched->idle_task) {
			/* the idle task isn't kept on the ready queue */
			prev->flags_state = TASK_SET_STATE(prev->flags_state, TASK_STATE_READY);
		} else {
			sched_readyq_insert_current_head(prev);
		}
	} else {
		assert(prev != sched->idle_task);
		if (TASK_TYPE_IS_TASK(prev->cfg->cfgflags_type)) {
			sched->current_part_cfg->part->last_real_task = NULL;
		}
	}

	/* handle pending partition shutdowns */
	if (sched->pending_part_mode_change != NULL) {
		sched_do_part_state_changes(sched);
	}

	/* pick next task */
	next = sched_find_next(sched->timepart, sched->idle_task);
	assert(next != NULL);
	assert(TASK_STATE_IS_READY(next->flags_state));
	next->flags_state = TASK_SET_STATE(next->flags_state, TASK_STATE_RUNNING);

	/* elevate priority once (for internal resources) */
	if (next->flags_state & TASK_FLAG_ELEV_PRIO) {
		next->flags_state &= ~TASK_FLAG_ELEV_PRIO;
		next->task_prio = next->cfg->elev_prio;
	}

	arch_set_idle(next == sched->idle_task);

	//printf("* cpu %d selects %p '%s/%s' as next task\n", arch_cpu_id(), next, next->cfg->part_cfg->name, next->cfg->name);

	if (next == prev) {
		/* no context switch, but scheduler activity: update user_sched_state */
		sched->user_sched_state->user_prio = next->task_prio;
		sched->user_sched_state->next_prio = sched->timepart->next_prio;
	} else {
		return sched_switch(sched, next);
	}

	return sched->regs;
}

static struct arch_reg_frame *sched_switch(struct sched_state *sched, struct task *next)
{
	const struct part_cfg *prev_part_cfg;
	const struct part_cfg *next_part_cfg;
	const struct task_cfg *next_cfg;

	assert(sched != NULL);
	assert(next != NULL);

	//printf("* cpu %d needs a switch from %p '%s'\n", arch_cpu_id(), sched->current_task, sched->current_task->cfg->name);
	arch_task_save(sched->regs, sched->fpu);

	sched->current_task = next;
	next_cfg = next->cfg;
	sched->regs = next_cfg->regs;
	sched->fpu = next_cfg->fpu;

	arch_task_restore(sched->regs, sched->fpu);

	prev_part_cfg = sched->current_part_cfg;
	next_part_cfg = next_cfg->part_cfg;
	/* update MPU and partition stack on partition switch */
	if (next_part_cfg != prev_part_cfg) {
		sched->current_part_cfg = next_part_cfg;
		arch_mpu_part_switch(next_part_cfg->mpu_part_cfg);
	}
	/* update global small data registers on TriCore */
	arch_set_partition_sda(next_part_cfg->sda1_base, next_part_cfg->sda2_base);

	/* monitor scheduling of real tasks for OS_GetTaskID */
	if (TASK_TYPE_IS_TASK(next_cfg->cfgflags_type)) {
		next_part_cfg->part->last_real_task = next;
	}

	arch_mpu_task_switch(next_cfg->mpu_task_cfg);

	/* update user_sched_state after context switch */
	sched->user_sched_state = next_part_cfg->user_sched_state;
	assert(sched->user_sched_state != NULL);
	sched->user_sched_state->taskid = next_cfg->task_id;
	sched->user_sched_state->user_prio = next->task_prio;
	sched->user_sched_state->next_prio = sched->timepart->next_prio;

	return sched->regs;
}

/** internal yield */
static void sched_yield(void)
{
	struct sched_state *sched;
	struct task *task;

	sched = current_sched_state();
	assert(sched != NULL);

	task = sched->current_task;
	assert(task != NULL);
	assert(TASK_STATE_IS_RUNNING(task->flags_state));

#ifdef SMP
	sched->reschedule |= 1U << arch_cpu_id();
#else
	sched->reschedule = 1;
#endif

	sched_readyq_insert_current_tail(task);
}

/** yield the current task */
void sys_yield(void)
{
	SET_RET(E_OK);
	sched_yield();
}

void sys_schedule(void)
{
	const struct task_cfg *cfg;
	struct sched_state *sched;
	struct task *task;
	unsigned int prio;

	sched = current_sched_state();
	assert(sched != NULL);
	assert(sched->current_task != NULL);

	task = sched->current_task;
	/* NOTE: the following test for TASK_FLAG_ELEV_PRIO uses the value
	 * from configuration to test if the task has an internal resource,
	 * because the flag is cleared in task->flags after initially increasing
	 * the priority
	 */
	cfg = task->cfg;
	if (cfg->cfgflags_type & TASK_CFGFLAG_ELEV_PRIO) {
		if (sched_curr_prio() != cfg->elev_prio) {
			SET_RET(E_OS_RESOURCE);
			return;
		}

		/* reset to base prio and preempt */
		prio = cfg->base_prio;
		task->task_prio = prio;
		sched->user_sched_state->user_prio = prio;
		/* indicate a pending elevation */
		task->flags_state |= TASK_FLAG_ELEV_PRIO;

#ifdef SMP
		sched->reschedule |= 1U << arch_cpu_id();
#else
		sched->reschedule = 1;
#endif
	}

	SET_RET(E_OK);
}

/** enforce scheduling */
void sys_fast_prio_sync(void)
{
	struct sched_state *sched;

	sched = current_sched_state();
	assert(sched != NULL);

#ifdef SMP
	sched->reschedule |= 1U << arch_cpu_id();
#else
	sched->reschedule = 1;
#endif
}

/** Get own scheduling priority */
unsigned int current_prio_get(void)
{
	unsigned int prio;

	prio = sched_curr_prio();

	return prio;
}

/** Set own scheduling priority */
/* NOTE: new priority must be bounded! */
void current_prio_set(unsigned int new_prio)
{
	struct sched_state *sched;

	sched = current_sched_state();
	assert(sched != NULL);

	assert(new_prio <= sched->current_part_cfg->max_prio);
	sched->user_sched_state->user_prio = new_prio;

	/* yield current task */
	sched_yield();
}

/** Replenish deadline */
void sys_replenish(timeout_t budget)
{
	const struct task_cfg *cfg;
	unsigned int adjust;
	GCC_NOWARN_UNINITIALIZED(time_t deadline);
	struct task *task;

	task = current_task();
	cfg = task->cfg;
	assert(cfg != NULL);

	if (cfg->capacity <= 0) {
		SET_RET(E_OK);	/* ERRNO: no deadline, no effect */
		return;
	}

	/* adjust deadline */
	if (budget >= 0) {
		/* NOTE: we need to adjust the expiry time by the timer's resolution
		 * to guarantee that we have *at least* waited the specified time.
		 */
		adjust = board_timer_resolution - 1;
		deadline = board_get_time() + budget + adjust;
	} else {
		deadline = INFINITY;
	}

	/* limit deadline to next release point */
	if (cfg->period > 0) {
		if ((deadline == INFINITY) ||
		    (deadline >= task->last_activation + cfg->period)) {
			SET_RET(E_OS_RESOURCE);	/* ERRNO: exceeds next release point */
			return;
		}
	}

	SET_RET(E_OK);

	/* change deadline */
	sched_deadline_change(deadline, task);
}

/** Get current system time */
void sys_gettime(void)
{
	time_t time;

	time = board_get_time();

	SET_RET64(time);
}

/** notify kernel on timer interrupt (passes current time in nanosecond) */
void kernel_timer(time_t now)
{
	struct sched_state *sched;
	struct task *task;
	list_t *node;

	sched = current_sched_state();
	assert(sched != NULL);

	/* switch time partitions */
	if (sched->next_tp_switch <= now) {
		tp_switch(sched);
	}

	/* expire timeouts */
next_timeout:
	if (!list_is_empty(&sched->timepart->timeoutq)) {
		node = __list_first(&sched->timepart->timeoutq);
		assert(node != NULL);
		task = list_entry(node, struct task, ready_and_timeoutq);
		assert(task != NULL);

		if (task->expiry_time <= now) {
			sched_timeout_expire(now, task);
			goto next_timeout;
		}
	}

	/* check deadlines */
next_deadline:
	if (!list_is_empty(&sched->timepart->deadlineq)) {
		node = __list_first(&sched->timepart->deadlineq);
		assert(node != NULL);
		task = list_entry(node, struct task, deadlineq);
		assert(task != NULL);

		if (unlikely(task->deadline <= now)) {
			sched_deadline_disable(task);
			/* notify HM */
			hm_async_task_error(task->cfg, HM_ERROR_DEADLINE_MISSED, 0);
			goto next_deadline;
		}
	}

	/* notify kernel to increment system timer counter */
	system_timer_increment();
}

/** Wait until next partition activation / release point. */
void sys_wait_periodic(void)
{
	const struct task_cfg *cfg;
	time_t expiry_time;
	struct task *task;

	task = current_task();
	cfg = task->cfg;
	assert(cfg != NULL);

	if (cfg->period <= 0) {
		SET_RET(E_OS_RESOURCE);	/* ERRNO: not a periodic task */
		return;
	}

	SET_RET(E_OK);

	if (cfg->capacity > 0) {
		/* deactivate previous deadline */
		sched_deadline_disable(task);
	}

	expiry_time = task->last_activation + cfg->period;
	task->expiry_time = expiry_time;

	/* go to sleep ... */
	list_node_init(&task->ready_and_timeoutq);
	#define ITER list_entry(__ITER__, struct task, ready_and_timeoutq)
	assert(cfg->timepart == current_sched_state()->timepart);
	list_add_sorted(&cfg->timepart->timeoutq, &task->ready_and_timeoutq, ITER->expiry_time >= expiry_time);
	#undef ITER

	sched_wait_internal(task, TASK_STATE_WAIT_ACT);
}

/** Change time partition schedule on target CPU */
void sys_schedule_change(unsigned int cpu_id, unsigned int schedule_id)
{
	const struct tpschedule_cfg *tpschedule;

	if (!(current_part_cfg()->flags & PART_FLAG_PRIVILEGED)) {
		SET_RET(E_OS_ACCESS);	/* ERRNO: partition privilege error */
		return;
	}
	if (cpu_id >= num_cpus) {
		SET_RET(E_OS_ID);
		return;
	}
	if (schedule_id >= num_tpschedules) {
		SET_RET(E_OS_ID);
		return;
	}
	tpschedule = &tpschedule_cfg[schedule_id];

	SET_RET(E_OK);

#ifdef SMP
	if (cpu_id != arch_cpu_id()) {
		ipi_enqueue(cpu_id, tpschedule, IPI_ACTION_SCHEDULE_CHANGE, 0);
		return;
	}
#endif

	schedule_change(tpschedule);
}

/** internal time partition schedule switch routine on current CPU */
void schedule_change(const struct tpschedule_cfg *next_tpschedule)
{
	struct sched_state *sched;

	assert(next_tpschedule != NULL);

	sched = current_sched_state();
	sched->wrap_tpwindow = next_tpschedule->start;
}

static void tp_switch(struct sched_state *sched)
{
	struct timepart_state *old_timepart;
	const struct tpwindow_cfg *win;

	/* find next window */
	win = sched->tpwindow;
	if (win->flags & TPWINDOW_FLAG_LAST) {
		/* wrap-around: look-up next schedule table */
		sched = current_sched_state();
		win = sched->wrap_tpwindow;
		assert(win != NULL);
	} else {
		win++;
	}

	/* switch time partition */
	assert(win->timepart < num_timeparts);

	sched->tpwindow = win;
	old_timepart = sched->timepart;
	sched->timepart = &core_cfg[arch_cpu_id()].timeparts[win->timepart];

#ifdef SMP
	sched->reschedule |= 1U << arch_cpu_id();
#else
	sched->reschedule = 1;
#endif

	/* set next window's expiry */
	assert(win->duration > 0);
	sched->last_tp_switch = sched->next_tp_switch;
	if ((win->flags & TPWINDOW_FLAG_RELEASE) != 0) {
		sched->timepart->last_release_point = sched->last_tp_switch;
	}
	sched->next_tp_switch = sched->last_tp_switch + win->duration;

	/* notify board */
	board_tp_switch(old_timepart->timepart_id, win->timepart, win->flags);

}

/** register a pending partition state change */
void sched_enqueue_part_state_change(
	struct part *part)
{
	struct sched_state *sched;

	assert(part != NULL);
	assert(part->cfg->cpu_id == arch_cpu_id());

	assert(part->next_pending_mode_change == NULL);

	/* enqueue partition for mode change */
	sched = current_sched_state();
	part->next_pending_mode_change = sched->pending_part_mode_change;
	sched->pending_part_mode_change = part;

#ifdef SMP
	sched->reschedule |= 1U << arch_cpu_id();
#else
	sched->reschedule = 1;
#endif
}

/** handle pending pending partition state changes */
static void sched_do_part_state_changes(
	struct sched_state *sched)
{
	struct part *part;

	assert(sched != NULL);
	assert(sched->pending_part_mode_change != NULL);

	/* dequeue partitions for mode change */
	part = sched->pending_part_mode_change;
	sched->pending_part_mode_change = NULL;

next_part:
	part_state_change(part);

	part = part->next_pending_mode_change;
	if (part != NULL) {
		part->next_pending_mode_change = NULL;
		goto next_part;
	}
}

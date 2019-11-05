/*
 * event.c
 *
 * Kernel event handling.
 *
 * azuepke, 2014-03-07: initial
 */

#include <kernel.h>
#include <assert.h>
#include <part.h>
#include <task.h>
#include <event.h>
#include <sched.h>
#include <hv_error.h>
#include <ipev.h>
#include <ipi.h>


void sys_ev_set(unsigned int task_id, evmask_t mask)
{
	const struct part_cfg *part_cfg;
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
	if (!(TASK_MAY_BLOCK(task->flags_state))) {
		SET_RET(E_OS_ACCESS);
		return;
	}

	err = ev_set(task, mask);
	SET_RET(err);
}

unsigned int ev_set(struct task *task, evmask_t mask)
{
	evmask_t clear;

	assert(task != NULL);

	if (TASK_STATE_IS_SUSPENDED(task->flags_state)) {
		/* task is not ready or waiting */
		return E_OS_STATE;
	}

	task->ev_pending |= mask;
	if (TASK_STATE_IS_WAIT_EV(task->flags_state) &&
	    ((arch_reg_frame_get_arg0(task->cfg->regs) & mask) != 0)) {
		/* be careful: out1 overwrites arg1 */
		clear = arch_reg_frame_get_arg1(task->cfg->regs);
		barrier();
		/* get current events */
		arch_reg_frame_set_out1(task->cfg->regs, task->ev_pending);
		/* clear events */
		task->ev_pending &= ~clear;
		/* careful again: return overwrites arg0 */
		arch_reg_frame_set_return(task->cfg->regs, E_OK);
		/* wake up task */
		sched_readyq_insert_tail(task);
	}

	return E_OK;
}

void sys_ev_get(unsigned int task_id)
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
	if (!(TASK_MAY_BLOCK(task->flags_state))) {
		SET_RET(E_OS_ACCESS);
		return;
	}

	if (TASK_STATE_IS_SUSPENDED(task->flags_state)) {
		/* task is not ready or waiting */
		SET_RET(E_OS_STATE);
		return;
	}

	SET_OUT1(task->ev_pending);
	SET_RET(E_OK);
}

void sys_ev_clear(evmask_t mask)
{
	struct task *task;

	task = current_task();
	if (!(TASK_MAY_BLOCK(task->flags_state))) {
		SET_RET(E_OS_ACCESS);
		return;
	}

	task->ev_pending &= ~mask;
	SET_RET(E_OK);
}

void sys_ev_wait_get_clear(evmask_t mask, evmask_t clear)
{
	struct task *task;

	task = current_task();
	if (!(TASK_MAY_BLOCK(task->flags_state))) {
		SET_RET(E_OS_ACCESS);
		return;
	}

	if ((task->ev_pending & mask) != 0) {
		/* get current events */
		SET_OUT1(task->ev_pending);
		/* clear events */
		task->ev_pending &= ~clear;
		SET_RET(E_OK);
		return;
	}

	/* save masks in register frame if not already saved at kernel entry */
	/* NOTE: on some archs, arguments and return value share the same regs! */
	/* the return code is set in ev_set() */
	SAVE_ARG0(mask);
	SAVE_ARG1(clear);
	sched_wait(task, TASK_STATE_WAIT_EV, -1);
}

void sys_ipev_set(unsigned int ipev_id)
{
	const struct part_cfg *part_cfg;
	const struct ipev_cfg *ipev;
	const struct task_cfg *task_cfg;
	unsigned int operating_mode;
	struct task *task;
	evmask_t mask;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);

	if (ipev_id >= part_cfg->num_ipevs) {
		SET_RET(E_OS_ID);
		return;
	}

	ipev = &part_cfg->ipevs[ipev_id];

	assert(ipev->global_task_id < num_tasks);
	task_cfg = task_get_task_cfg(ipev->global_task_id);
	task = task_cfg->task;
	assert(task != NULL);
	assert(TASK_MAY_BLOCK(task->flags_state));

	/* FIXME: there is no way to return an error in the IPI case if the target
	 * partition is not running, so we return E_OK here.
	 */
	SET_RET(E_OK);

	operating_mode = task_cfg->part_cfg->part->operating_mode;
	if (operating_mode == PART_OPERATING_MODE_IDLE) {
		/* target partition not ready to receive */
		return;
	}

#ifdef SMP
	if (task_cfg->cpu_id != arch_cpu_id()) {
		/* Use the NOERR variant, otherwise a partition can easily overflow
		 * the error handling records of the target partition.
		 */
		ipi_enqueue(task_cfg->cpu_id, task, IPI_ACTION_EVENT_NOERR, ipev->mask_bit);
		return;
	}
#endif

	assert(ipev->mask_bit < 32);	/* 32 for 32 bits */
	mask = 1U << ipev->mask_bit;

	(void)ev_set(task, mask);
}

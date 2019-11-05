/*
 * wq.c
 *
 * ARINC compatible abtract wait queues.
 *
 * azuepke, 2014-09-05: initial
 */

#include <kernel.h>
#include <assert.h>
#include <wq.h>
#include <hv_error.h>
#include <task_state.h>
#include <part.h>
#include <sched.h>
#include <task.h>
#include <ipi.h>
#include <rpc.h>


/** initialize all wait queues
 *
 * NOTE: called ONCE during system startup
 */
__init void wq_init_all(void)
{
	const struct part_cfg *part_cfg;
	unsigned int i, j;
	struct wq *wq;

	for (i = 0; i < num_partitions; i++) {
		part_cfg = part_get_part_cfg(i);

		for (j = 0; j < part_cfg->num_wqs; j++) {
			wq = &part_cfg->wqs[j];
			assert(wq != NULL);

			list_head_init(&wq->waitq);

			/* state initialized to zero at boot */
			assert(wq->state == WQ_STATE_CLOSED);

			wq->cpu_id = part_cfg->cpu_id;
		}
	}
}

/** close a wait queue -- called on partition shutdown/reboot
 *
 * Called after all the partition's tasks have been terminated,
 * so no tasks can be on the wait queue
 */
void wq_close(struct wq *wq)
{
	assert(wq != NULL);

	assert(list_is_empty(&wq->waitq));
	wq->state = WQ_STATE_CLOSED;
}

/** Set wait queue discipline */
void sys_wq_set_discipline(
	unsigned int wq_id,
	unsigned int discipline,
	uint32_t *user_state)
{
	const struct part_cfg *part_cfg;
	const struct wq_cfg *cfg;
	unsigned int err;
	struct wq *wq;

	if ((discipline != WQ_DISCIPLINE_FIFO) &&
	    (discipline != WQ_DISCIPLINE_PRIO)) {
		SET_RET(E_OS_VALUE);	/* ERRNO: Invalid discipline */
		return;
	}

	part_cfg = current_part_cfg();
	if (wq_id >= part_cfg->num_wqs) {
		SET_RET(E_OS_ID);
		return;
	}

	/* check if wait queue is usable */
	cfg = &part_cfg->wq_cfgs[wq_id];
	if (cfg->link == NULL) {
		SET_RET(E_OS_NOFUNC);	/* ERRNO: Wait queue is not connected */
		return;
	}

	wq = &part_cfg->wqs[wq_id];
	if ((wq->state == WQ_STATE_READY) && !list_is_empty(&wq->waitq)) {
		SET_RET(E_OS_STATE);	/* ERRNO: Wait queue already in use */
		return;
	}

	/* check user space address of user_state */
	err = kernel_check_user_addr(user_state, sizeof(*user_state));
	if (err != E_OK) {
		/* no range matched */
		SET_RET(err);
		return;
	}

	wq->discipline = discipline;
	wq->user_state = user_state;
	wq->state = WQ_STATE_READY;

	SET_RET(E_OK);
}

/** Wait on a wait queue */
void __sys_wq_wait(
	unsigned int wq_id,
	uint32_t compare,
	timeout_t timeout,
	void *prio_casted_as_ptr)
{
	unsigned int prio = (unsigned int)prio_casted_as_ptr;
	const struct part_cfg *part_cfg;
	struct task *task;
	uint32_t state;
	struct wq *wq;

	part_cfg = current_part_cfg();
	if (wq_id >= part_cfg->num_wqs) {
		SET_RET(E_OS_ID);
		return;
	}

	/* check if wait queue is usable */
	wq = &part_cfg->wqs[wq_id];
	if (wq->state != WQ_STATE_READY) {
		SET_RET(E_OS_NOFUNC);	/* ERRNO: Wait queue is not initialized */
		return;
	}

	/* compare */
	barrier();
	/* NOTE: this is a direct access to user space! Adspace checked before */
	state = *wq->user_state;
	barrier();

	if (state != compare) {
		SET_RET(E_OS_VALUE);	/* ERRNO: Comparation failed */
		return;
	}

	/* enqueue in waitq */
	if (timeout == 0) {
		SET_RET(E_OS_TIMEOUT);	/* ERRNO: No timeout */
		return;
	}

	task = current_task();
	if (!(TASK_MAY_BLOCK(task->flags_state))) {
		SET_RET(E_OS_ACCESS);
		return;
	}

	task->wait_prio = prio;
	list_node_init(&task->waitq);

	if (wq->discipline == WQ_DISCIPLINE_PRIO) {
		#define ITER list_entry(__ITER__, struct task, waitq)
		list_add_sorted(&wq->waitq, &task->waitq, ITER->wait_prio < prio);
		#undef ITER
	} else {
		list_add_last(&wq->waitq, &task->waitq);
	}

	/* E_OK is overwritten when the timeout expires */
	SET_RET(E_OK);

	/* go to sleep */
	sched_wait(task, TASK_STATE_WAIT_WQ, timeout);
}


/** Sleep until timeout expires */
void sys_sleep(timeout_t timeout)
{
	struct task *task;

	task = current_task();
	if (!(TASK_MAY_BLOCK(task->flags_state))) {
		SET_RET(E_OS_ACCESS);
		return;
	}
	if (timeout == 0) {
		SET_RET(E_OS_TIMEOUT);
		return;
	}

	/* NOTE: we init the node as head to allow safe list deletion */
	list_head_init(&task->waitq);

	/* E_OK is overwritten when the timeout expires */
	SET_RET(E_OK);

	/* go to sleep */
	sched_wait(task, TASK_STATE_WAIT_WQ, timeout);
}

void wq_wake(
	struct wq *wq,
	unsigned int count)
{
	struct task *task;
	list_t *node;

	assert(wq != NULL);

	/* wake "count" waiters */
	while (count > 0) {
		count--;

		/* get first task from list */
		node = list_remove_first(&wq->waitq);
		if (node == NULL) {
			break;
		}
		task = list_entry(node, struct task, waitq);
		assert(task != NULL);
		assert(TASK_STATE_IS_WAIT_WQ(task->flags_state));

		/* wake up task: remove from timeout queue */
		list_del(&task->ready_and_timeoutq);

		sched_readyq_insert_tail(task);
	}
}

/** Notify a wait queue */
void sys_wq_wake(
	unsigned int wq_id,
	unsigned int count)
{
	const struct part_cfg *part_cfg;
	const struct wq_cfg *cfg;
	struct wq *wq;

	part_cfg = current_part_cfg();
	if (wq_id >= part_cfg->num_wqs) {
		SET_RET(E_OS_ID);
		return;
	}

	/* check if wait queue is usable */
	cfg = &part_cfg->wq_cfgs[wq_id];
	wq = cfg->link;
	if (wq == NULL) {
		SET_RET(E_OS_NOFUNC);	/* ERRNO: Wait queue is not connected */
		return;
	}

	SET_RET(E_OK);

	if (wq->state == WQ_STATE_READY) {
#ifdef SMP
		/* wake targets tasks on other core? */
		if (wq->cpu_id != arch_cpu_id()) {
			if (count > NUM_WAITERS) {
				count = NUM_WAITERS;
			}
			ipi_enqueue(wq->cpu_id, wq, IPI_ACTION_WQ_WAKE, count);
			return;
		}
#endif

		wq_wake(wq, count);
	}
}

/** Cancel sleeping / wake up a task waiting task in a wait queue */
void sys_unblock(
	unsigned int task_id)
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

	task = &part_cfg->tasks[task_id];
	assert(task != NULL);
	if (!TASK_STATE_IS_WAIT_WQ(task->flags_state) &&
	    !TASK_STATE_IS_WAIT_SEND(task->flags_state)) {
		SET_RET(E_OS_NOFUNC);	/* ERRNO: task is currently not waiting */
		return;
	}

	/* cancel RPC */
	if (TASK_STATE_IS_WAIT_SEND(task->flags_state)) {
		rpc_cancel(task);
	}

	/* wake task: remove from wait queue and from timeout queue ... */
	list_del(&task->waitq);
	list_del(&task->ready_and_timeoutq);

	/* set new error code in registers */
	cfg = task->cfg;
	regs = cfg->regs;
	arch_reg_frame_set_return(regs, E_OS_STATE);

	SET_RET(E_OK);

	/* ... and put onto ready queue again */
	sched_readyq_insert_tail(task);
}

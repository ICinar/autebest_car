/*
 * rpc.c
 *
 * Kernel RPC handling.
 *
 * azuepke, 2016-01-08: initial
 */

#include <kernel.h>
#include <assert.h>
#include <rpc.h>
#include <part_state.h>
#include <task.h>
#include <hv_error.h>
#include <sched.h>


/** initialize per-task RPC queues
 *
 * NOTE: called during system startup only, for each RPC object
 */
__init void rpc_init(struct rpc *rpc)
{
	assert(rpc != NULL);

	list_head_init(&rpc->sendq);
	list_head_init(&rpc->recvq);
}

/** RPC call system call */
void sys_rpc_call(
	unsigned int rpc_id,
	unsigned long send_arg,
	timeout_t timeout)
{
	const struct part_cfg *part_cfg;
	const struct rpc_cfg *rpc_cfg;
	unsigned int operating_mode;
	unsigned int reply_id;
	unsigned int prio;
	struct task *receiver;
	struct task *sender;
	struct rpc *rpc;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);

	if (rpc_id >= part_cfg->num_rpcs) {
		SET_RET(E_OS_ID);	/* ERRNO: invalid RPC ID */
		return;
	}

	rpc_cfg = &part_cfg->rpcs[rpc_id];

	/* RPC receiving hook */
	receiver = rpc_cfg->task;
	assert(receiver != NULL);

	/* both sender and receiver must reside on the same processor! */
	assert(receiver->cfg->cpu_id == arch_cpu_id());

	operating_mode = receiver->cfg->part_cfg->part->operating_mode;
	if (operating_mode == PART_OPERATING_MODE_IDLE) {
		SET_RET(E_OS_STATE);	/* ERRNO: target partition not ready */
		return;
	}

	if (receiver->pending_activations >= 0xff) {
		SET_RET(E_OS_LIMIT);	/* ERRNO: RPC queue overflow */
		return;
	}

	/* non-blocking RPC call */
	if ((timeout == 0) && !TASK_STATE_IS_SUSPENDED(receiver->flags_state)) {
		SET_RET(E_OS_TIMEOUT);	/* ERRNO: No timeout */
		return;
	}

	/* register RPC */
	sender = current_task();
	assert(sender->rpc_task == NULL);
	sender->rpc_task = receiver;

	/* RPC call priority */
	prio = sender->task_prio;
	if (prio < rpc_cfg->prio) {
		prio = rpc_cfg->prio;
	}
	if (prio < receiver->cfg->base_prio) {
		prio = receiver->cfg->base_prio;
	}
	sender->wait_prio = prio;

	rpc = receiver->cfg->rpc;
	assert(rpc != NULL);

	/* the reply ID is the global task ID of the sender */
	reply_id = task_get_global_id(sender);

	if (TASK_STATE_IS_SUSPENDED(receiver->flags_state)) {
		/* fast path -- send immediately, not enqueued on send queue,
		 * only on receive queue. Also, the send queue must be empty
		 * if the receiver is in SUSPENDED state.
		 */
		assert(list_is_empty(&rpc->sendq));
		assert(receiver->pending_activations == 0);

		list_add_last(&rpc->recvq, &sender->waitq);

		/* NOTE: waiters on the RPC receive queue have infinite timeout */
		sched_wait(sender, TASK_STATE_WAIT_RECV, -1);

		/* activate hook and pass message */
		task_prepare(receiver);
		arch_reg_frame_set_arg0(receiver->cfg->regs, reply_id);
		arch_reg_frame_set_arg1(receiver->cfg->regs, send_arg);
		receiver->task_prio = prio;
		sched_readyq_insert_tail(receiver);
	} else {
		/* slow path -- enqueue on send queue */
		assert(timeout != 0);
		receiver->pending_activations++;

		list_add_last(&rpc->sendq, &sender->waitq);

		/* save send_arg in register frame if not already saved at kernel entry
		 * and keep reply_id here as well.
		 */
		SET_ARG0(reply_id);
		SAVE_ARG1(send_arg);

		sched_wait(sender, TASK_STATE_WAIT_SEND, timeout);
	}

	/* the return code is set on RPC reply */
}

/** start processing of next RPC (called during task re-activation) */
void rpc_next(
	struct task *receiver,
	struct rpc *rpc)
{
	unsigned long send_arg;
	unsigned int reply_id;
	struct task *sender;
	list_t *node;

	assert(receiver != NULL);
	assert(rpc != NULL);
	assert(rpc == receiver->cfg->rpc);

	/* get first waiting task from RPC send queue */
	assert(list_first(&rpc->sendq) != NULL);
	node = __list_remove_first(&rpc->sendq);
	sender = list_entry(node, struct task, waitq);
	assert(sender != NULL);
	assert(TASK_STATE_IS_WAIT_SEND(sender->flags_state));
	assert(sender->rpc_task == receiver);

	/* move sender to RPC receive queue */
	list_add_last(&rpc->recvq, &sender->waitq);
	sender->flags_state = TASK_SET_STATE(sender->flags_state, TASK_STATE_WAIT_RECV);

	/* also remove from any timeout queue */
	list_del(&sender->ready_and_timeoutq);
	/* NOTE: we init the node as head again to allow safe list deletion */
	list_head_init(&sender->ready_and_timeoutq);

	/* copy RPC arguments + set prio */
	reply_id = arch_reg_frame_get_arg0(sender->cfg->regs);
	arch_reg_frame_set_arg0(receiver->cfg->regs, reply_id);

	send_arg = arch_reg_frame_get_arg1(sender->cfg->regs);
	arch_reg_frame_set_arg1(receiver->cfg->regs, send_arg);

	receiver->task_prio = sender->wait_prio;
}

/** internal RPC wakeup routine */
static void rpc_set_reply_and_wake(
	struct task *task,
	unsigned long reply_arg,
	unsigned int error)
{
	assert(task != NULL);
	assert(TASK_STATE_IS_WAIT_SEND(task->flags_state) ||
	       TASK_STATE_IS_WAIT_RECV(task->flags_state));

	assert(task->rpc_task != NULL);
	task->rpc_task = NULL;

	/* wake up task: remove from RPC send / receive queues and timeout queue */
	list_del(&task->waitq);
	list_del(&task->ready_and_timeoutq);

	arch_reg_frame_set_return(task->cfg->regs, error);
	arch_reg_frame_set_out1(task->cfg->regs, reply_arg);
	/* wake up caller, inserts at HEAD! */
	sched_readyq_insert_head(task);	/* FIXME: enqueue at HEAD -> also on errors? */
}


/** RPC reply system call */
void sys_rpc_reply(
	unsigned int reply_id,
	unsigned long reply_arg,
	int terminate)
{
	struct task *rpc_task;
	struct task *task;

	/* find waiting task to reply to. reply_id refers to the global task ID */
	if (reply_id >= num_tasks) {
		SET_RET(E_OS_ID);		/* ERRNO: invalid task ID */
		return;
	}
	task = task_get_task_cfg(reply_id)->task;
	rpc_task = task->rpc_task;
	if (rpc_task == NULL) {
		SET_RET(E_OS_ID);		/* ERRNO: task not doing RPC */
		return;
	}
	if (!TASK_STATE_IS_WAIT_RECV(task->flags_state)) {
		SET_RET(E_OS_ID);		/* ERRNO: task not waiting for RPC reply */
		return;
	}
	if (rpc_task->cfg->part_cfg != current_part_cfg()) {
		SET_RET(E_OS_ID);		/* ERRNO: RPC task not in caller's partition */
		return;
	}

	SET_RET(E_OK);

	/* reply to caller */
	rpc_set_reply_and_wake(task, reply_arg, E_OK);

#ifndef NDEBUG
	assert(rpc_task->cfg->rpc != NULL);
	if (task->pending_activations > 0) {
		assert(list_first(&rpc_task->cfg->rpc->sendq) != NULL);
	} else {
		assert(list_first(&rpc_task->cfg->rpc->sendq) == NULL);
	}
#endif

	if (terminate) {
		/* will start the next RPC */
		task_terminate_self(current_task());
	}
}

/** safely remove a task from RPC send or recv queue */
void rpc_cancel(struct task *task)
{
	struct task *rpc_task;

	assert(task != NULL);
	assert(TASK_STATE_IS_WAIT_SEND(task->flags_state) ||
	       TASK_STATE_IS_WAIT_RECV(task->flags_state));

	rpc_task = task->rpc_task;
	assert(rpc_task != NULL);
	task->rpc_task = NULL;

	if (TASK_STATE_IS_WAIT_SEND(task->flags_state)) {
		assert(rpc_task->pending_activations > 0);
		rpc_task->pending_activations--;
	}

	/* removal from RPC send / recv queue is done by caller of this function */
}

/** abort all queued RPC, called on partition shutdown */
void rpc_abort(struct task *rpc_task, struct rpc *rpc)
{
	struct task *task;
	list_t *node;

	assert(rpc != NULL);
	assert(rpc_task != NULL);

	/* kick all waiting tasks from the send queue ... */
	while ((node = list_first(&rpc->sendq)) != NULL) {
		task = list_entry(node, struct task, waitq);
		assert(task != NULL);
		assert(task->rpc_task == rpc_task);

		rpc_set_reply_and_wake(task, 0, E_OS_STATE);
		assert(rpc_task->pending_activations > 0);
		rpc_task->pending_activations--;
	}
	assert(rpc_task->pending_activations == 0);

	/* ... and from the receive queue */
	while ((node = list_first(&rpc->recvq)) != NULL) {
		task = list_entry(node, struct task, waitq);
		assert(task != NULL);
		assert(task->rpc_task == rpc_task);

		rpc_set_reply_and_wake(task, 0, E_OS_STATE);
	}
}

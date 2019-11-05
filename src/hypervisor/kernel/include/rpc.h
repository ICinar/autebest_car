/*
 * rpc.h
 *
 * Kernel RPC handling.
 *
 * azuepke, 2016-01-08: initial
 */

#ifndef __RPC_H__
#define __RPC_H__

#include <rpc_state.h>
#include <hv_types.h>

/** RPC call system call */
__tc_fastcall void sys_rpc_call(
	unsigned int rpc_id,
	unsigned long send_arg,
	timeout_t timeout);

/** RPC reply system call */
__tc_fastcall void sys_rpc_reply(
	unsigned int rpc_id,
	unsigned long reply_arg,
	int terminate);

/** initialize per-task RPC queues */
void rpc_init(struct rpc *rpc);

/** start processing of next RPC */
void rpc_next(struct task *receiver, struct rpc *rpc);

/** safely remove a task from RPC send or recv queue */
void rpc_cancel(struct task *task);

/** abort all queued RPC, called on partition shutdown */
void rpc_abort(struct task *rpc_task, struct rpc *rpc);

#endif

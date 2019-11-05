/*
 * rpc_state.h
 *
 * Kernel RPC state.
 *
 * azuepke, 2016-01-08: initial
 */

#ifndef __RPC_STATE_H__
#define __RPC_STATE_H__

#include <stdint.h>
#include <list.h>

/* forward declaration */
struct task;

/** RPC configuration (describes caller's side) */
struct rpc_cfg {
	struct task *task;		/* associated hook to invoke */
	uint8_t prio;			/* priority to elevate */
	uint8_t padding1;
	uint16_t padding2;
};


/** RPC runtime data (wait queue) */
struct rpc {
	list_t sendq;				/* list of waiting tasks to send */
	list_t recvq;				/* list of waiting tasks to recv */
};

#endif

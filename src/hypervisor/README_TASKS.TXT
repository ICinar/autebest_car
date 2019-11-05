# README_TASKS.TXT
#
# azuepke,  2015-12-22: initial

This overview lists the different task types of the kernel, their scheduler
states and other state flags used internally.


Task Types
===========

Task Type		Activate	Terminate	Chain	Block	RPC^1	Description
- Hook			(indirect)	yes			no		yes/no	yes		<hook>
- ISR			(via IRQ)	yes			no		yes/no	yes		<isr>
- bas. Task		yes			yes			yes		no		yes		<task>
- ext. Task		yes^2		yes			yes		yes		yes		<task>
- Invokable		(via RPC)	yes			no		yes/no	yes		<invokable>

^1: Integrater must take care that called RPC tasks does not block
^2: some tasks require dynamic setting of their initial register context

#define TASK_TYPE_HOOK				0x00	// task is a hook
#define TASK_TYPE_ISR				0x01	// task is a cat 2 ISR
#define TASK_TYPE_TASK				0x02	// task is basic or extended task
#define TASK_TYPE_INVOKABLE			0x03	// task is an RPC call target

#define TASK_TYPE_MASK				0x03


Task Attributes (kept in ROM)
==============================

#define TASK_CFGFLAG_ELEV_PRIO		0x80	// elevate priority on scheduling
#define TASK_CFGFLAG_ACTIVATABLE	0x40	// task is activatable (has entry point and stack)
#define TASK_CFGFLAG_CALLABLE		0x20	// task is callable via RPC
#define TASK_CFGFLAG_ISR_UNMASK		0x10	// ISR's interrupt source is unmasked at partition start
#define TASK_CFGFLAG_MAYBLOCK		0x08	// indicate that a task may block
#define TASK_CFGFLAG_UNUSED04		0x04	// unused

Together with the task type, we keep these bits in task_cfg::cfgflags_type.


Task Scheduling States
=======================

Task States	Readyq	Timeoq	Wq		Deadl?	Description
RUNNING		no		no		no		yes		currently running
READY		yes		no		no		yes		waiting to run
WAIT_RPC	no		yes		rpcq	yes		blocked in RPC
WAIT_EV		no		yes		self	yes		blocked in event
WAIT_WQ		no		yes		waitq	yes		blocked on waitqueue
WAIT_ACT	no		yes		no		yes		waiting for activation
SUSPENDED	no		no		no		no		suspended / dead

#define TASK_STATE_RUNNING			0x07	// Current task, not on ready queue
#define TASK_STATE_READY			0x06	// Ready, task on the ready queue
#define TASK_STATE_UNUSED5			0x05	// unused
#define TASK_STATE_WAIT_RPC			0x04	// Task waiting for RPC return
#define TASK_STATE_WAIT_EV			0x03	// Task waiting for an event
#define TASK_STATE_WAIT_WQ			0x02	// Task waiting on a wait queue
#define TASK_STATE_WAIT_ACT			0x01	// Task waiting for activation (delayed start)
#define TASK_STATE_SUSPENDED		0x00	// Task suspended / terminated

#define TASK_STATE_MASK				0x07


Task Flags (kept in RAM)
=========================

#define TASK_FLAG_ELEV_PRIO			0x80	// elevate priority on scheduling
#define TASK_FLAG_UNUSED40			0x40	// unused
#define TASK_FLAG_UNUSED20			0x20	// unused
#define TASK_FLAG_UNUSED10			0x10	// unused
#define TASK_FLAG_UNUSED08			0x08	// unused

Together with the task state, we keep these bits in task::flags_state.

# README_RPC.TXT
#
# azuepke, 2016-01-27: initial
# azuepke, 2016-02-05: all tasks in a partition can reply


Synchronous Remote Procedure Calls
===================================

The kernel offers a versatile synchronous RPC mechanism with blocking send
and receive operations with minimalistic message passing. A RPC comprises
the operations the CALL and REPLY. A CALL operation activates a special task
called an INVOKABLE in the same or a different partition. If multiple tasks
do a CALL to the same INVOKABLE, the callers are queued in FIFO order.
The INVOKABLE receives the caller's RPC message as parameter and may REPLY
immediately on task termination or later. The RPC implementation allows
multiple callers waiting for replies. Additionally, any task, hook, or ISR
in the INVOKABLE's partition can reply to an ongoing CALL.


RPC Configuration
------------------

On the receiver's side, INVOKABLES are defined like other tasks, ISRs, or hooks
using the <invokable>-XML entries:

    <invokable name="rpc_task" prio="123" regs="yes" fpu="no" contexts="5" blocking="yes">
        <invoke entry="rpc_func" arg="" stack="__stack" stack_size="256"/>
    </invokable>

The INVOKABLE is implemented as a special task type which allows blocking,
but cannot be addressed by normal task API calls like sys_task_activate().
Alarms or schedule tables cannot activate an INVOKABLE either.

On the caller's side, RPC is configured by RPC call entries:

    <rpc name="RPC_name" partition="part_name" invokable="rpc_task" prio="42"/>

These entries refer to the INVOKABLE's partition and task name and also provide
a minimum call priority like an internal OSEK resource.


RPC API
--------

On the caller's side, an RPC is performed by calling to the configured RPC
entry, which are enumerated by a per-partition RPC ID:

    unsigned int sys_rpc_call(
        unsigned int rpc_id,
        unsigned long send_arg,
        timeout_t timeout,
        unsigned long *recv_arg);

The caller can send and receive a single pointer-sized parameter. Also,
the caller can specify the maximum blocking time for the send phase of the call.
While blocked for send, the caller can be unblocked via sys_unblock().
The timeout for the receive phase (waiting for a REPLY) is always infinite
and cannot be unblocked.

On an incoming RPC, the INVOKABLE is invoked with the following parameter
signature:

    void rpc_task(
        unsigned int reply_id,
        unsigned long send_arg);

The first parameter defines an ID referencing the caller for a REPLY operation,
and the second parameter is the caller's transmitted argument. Unlike other
task types, the "arg" attribute in the <invoke> block in the configuration
is always ignored and overwritten by the REPLY ID.

To REPLY, the INVOKABLE (or any other task, hook or ISR in the INVOKABLE's
partition) uses the following operation:

    unsigned int sys_rpc_reply(
        unsigned int reply_id,
        unsigned long reply_arg,
        int terminate);

The first two parameters define the REPLY ID and the argument to return to
the caller. The last parameter allows implicit termination on successful REPLY
if set to a non-zero value. If the INVOKABLE terminates normally using
sys_task_terminate() or sys_task_chain(), no REPLY operation is performed
and the caller remains waiting in receiving (waiting for REPLY) state.


Priority Ceiling Protocol
--------------------------

On a CALL operation, the activated INVOKABLE inherits the maximum of the
following three priorities:
- the caller's current scheduling priority,
- the configured RPC priority, and
- the INVOKABLE's configured priority.

The configurable RPC priority on the caller's side allow to implement
a priority ceiling protocol to synchronize caller's of a single partition.
The priority of the INVOKABLE allows the same on the INVOKABLE's side.

The integrator needs to be aware of priority ceiling protocols and must not
set the INVOKABLE's partition maximum priority below the maximum of the
priorities on the caller's side.


Scheduling Implications
------------------------

RPC deviates from standard scheduling assumptions that tasks are always
enqueued at the end of their priorities' ready queue. While a CALL operation
always places the INVOKABLE at the end of its ready queue, the REPLY operation
does not. Instead, a REPLY operation places the woken caller at the beginning
of its ready queue!

This allows us to implement AUTOSAR's CallTrustedFunction() on top of RPC.
AUTOSAR expects no changes to task scheduling when calling a trusted function,
so we cannot allow to put the task at the end of its ready queue when having
multiple tasks at the same priority. In this case, it's also recommended
to use specify a timeout of zero in the CALL operation and immediately REPLY.


Multicore Considerations
-------------------------

All tasks involved in RPC must execute on the same processor, but may reside
in different time partitions. For cross CPU communication, asynchronous
IPC mechanisms like wait queues or inter partition events suit better.


Deadlock Detection
-------------------

Deadlocks can happen, for example if an RPC INVOKABLE calls itself or
two INVOKABLES try to CALL each other recursively (e.g. A -> B -> A).
The current RPC design does not allow deadlock detection in O(1), so the last
CALL will always block with the specified timeout.

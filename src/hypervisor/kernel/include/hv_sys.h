/*
 * hv_sys.h
 *
 * Kernel user API.
 *
 * azuepke, 2013-11-25: initial version for MPU kernel
 * azuepke, 2015-06-22: added full API documentation
 */

#ifndef __HV_SYS_H__
#define __HV_SYS_H__

#include <stdint.h>
#include <hv_types.h>
#include <hv_compiler.h>

/* NOTE: all functions are prefixed with sys_ */

/** Abort calling task and raise partition error
 *
 * This service terminates the calling task
 * and injects the partition error \a HM_ERROR_ABORT into health monitoring.
 * Neither the error nor the exception hook will be activated.
 *
 * \note This function never returns to the caller.
 *
 * \see sys_task_terminate()
 * \see sys_task_terminate_other()
 * \see sys_task_chain()
 */
__syscall void sys_abort(void) __noreturn;

/** Print character to system console
 *
 * A successful call to this function prints a character to the system console.
 * The system console is acting in polling mode. If the serial FIFO is full,
 * the function returns an error.
 *
 * \param [in] c			Character to print
 *
 * \retval E_OK				Success
 * \retval E_OS_NOFUNC		Serial FIFO is busy, try again later
 */
__syscall unsigned int sys_putchar(const char c);

/** Null system call
 *
 * This call has no effect.
 *
 * \note This function is provided for benchmarking purposes only.
 */
__syscall void sys_null(void);

/** Get current CPU ID
 *
 * A call to this function returns the ID of the CPU the caller is currently
 * executing on.
 *
 * \return Current CPU ID
 */
__syscall unsigned int sys_cpu_id(void);

/** Get current task, ISR, or Hook ID
 *
 * This service returns the partition local task ID
 * of the currently executing task, ISR, or hook
 * by using a system call.
 *
 * \return Task ID of current task, ISR, or hook.
 *
 * \see sys_fast_task_self()
 * \see sys_task_current()
 * \see sys_task_isrid()
 * \see user_error_state_t::task_id
 */
__syscall unsigned int sys_task_self(void);

/** Get current task, ISR, or Hook ID
 *
 * This service returns the partition local task ID
 * of the currently executing task, ISR, or hook
 * without system call overhead.
 *
 * \return Task ID of current task, ISR, or hook.
 *
 * \see sys_fast_task_self()
 * \see sys_task_current()
 * \see sys_task_isrid()
 * \see user_error_state_t::task_id
 */
unsigned int sys_fast_task_self(void);

/** Get task ID of last running task
 *
 * When an ISR or hook calls this function, the ID refers to the last preempted
 * task, or -1 if the partition was idle before.
 * When a task calls this function, the return value is always the task ID.
 *
 * \return Task ID of last task (not ISR or hook), or -1 when no task was scheduled
 *
 * \see sys_task_self()
 * \see sys_fast_task_self()
 * \see sys_task_isrid()
 * \see user_error_state_t::task_id
 */
__syscall unsigned int sys_task_current(void);

/** Get task ID of currently running ISR
 *
 * When an ISR calls this function, the returned ID reflects the ISR's task ID.
 * When a normal task or hook calls this, the returned ID is -1.
 *
 * \return Task ID of current ISR, or -1 when called from a task or hook
 *
 * \see sys_task_self()
 * \see sys_fast_task_self()
 * \see sys_task_current()
 * \see user_error_state_t::task_id
 */
__syscall unsigned int sys_task_isrid(void);


/** Create task in the caller's partition and assign register context
 *
 * A successful call to this function activates a non-activatable task
 * in SUSPENDED state referenced by its ID \a task_id in the caller's partition.
 * The task uses scheduling priority \a prio and user provided registers:
 * the entry point is \a entry, the initial stack is \a stack, the initial
 * arguments are \a arg0 and \a arg1.
 *
 * \param [in] task_id		ID of the task to create
 * \param [in] prio			Task priority
 * \param [in] entry		Entry point
 * \param [in] stack		Initial stack pointer
 * \param [in] arg0			First initial argument
 * \param [in] arg1			Second initial argument
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid task ID
 * \retval E_OS_ACCESS		Task ID refers to an ISR or hook
 * \retval E_OS_ACCESS		Task ID refers to an activatable task
 * \retval E_OS_STATE		Task is already activated
 * \retval E_OS_VALUE		Priority \a prio out of limits
 * \retval E_OS_ILLEGAL_ADDRESS	Invalid stack
 *
 * \see sys_task_activate()
 * \see sys_task_delayed_activate()
 * \see sys_task_chain()
 */
__syscall unsigned int sys_task_create(
	unsigned int task_id,
	unsigned int prio,
	void *entry,
	void *stack,
	void *arg0,
	void *arg1);

/** Terminate current task, ISR, or hook
 *
 * A call to this function always terminates the calling task, ISR, or hook.
 * If a task or hook has pending activations, the task or hook is immediately
 * activated again and begins scheduling at its configured start priority.
 *
 * \note This function never returns to the caller.
 *
 * \see sys_abort()
 * \see sys_task_chain()
 * \see sys_task_terminate_other()
 */
__syscall void sys_task_terminate(void);

/** Terminate other task, ISR, or hook
 *
 * A call to this function terminates the task, ISR, or hook \a task_id
 * in the caller's partition, except for the currently executing task.
 * A task or hook is immediately activated again on its configured start
 * priority if it has pending activations.
 *
 * \param [in] task_id		ID of the task to terminate
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid task ID
 * \retval E_OS_STATE		Task refers to the calling task
 * \retval E_OS_NOFUNC		Task is already terminated
 *
 * \see sys_abort()
 * \see sys_task_terminate()
 * \see sys_task_chain()
 */
__syscall unsigned int sys_task_terminate_other(unsigned int task_id);

/** Terminate current task and activate another task
 *
 * A successful call to this function terminates the current task and
 * activates another task \a task_id instead.
 * If \a task_id refers to the caller, the caller is immediately activated
 * again on its configured start priority.
 *
 * \param [in] task_id		ID of the task to activate
 *
 * \retval E_OS_ID			Invalid task ID
 * \retval E_OS_ACCESS		Task refers to an ISR or hook
 * \retval E_OS_ACCESS		Task is a non-activatable task
 *
 * \note On success, this function does not returns to the caller.
 *
 * \see sys_abort()
 * \see sys_task_terminate()
 * \see sys_task_terminate_other()
 * \see sys_task_create()
 * \see sys_task_activate()
 * \see sys_task_delayed_activate()
 */
__syscall unsigned int sys_task_chain(unsigned int task_id);

/** Activate a task
 *
 * A successful call to this function activates an activatable task
 * in SUSPENDED state referenced by its ID \a task_id in the caller's partition.
 * The task uses its configured start priority and configured registers.
 * If the task is not in SUSPENDED state, the activation is recorded as
 * pending activation.
 *
 * \param [in] task_id		ID of the task to activate
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid task ID
 * \retval E_OS_ACCESS		Task refers to an ISR or hook
 * \retval E_OS_ACCESS		Task is a non-activatable task
 * \retval E_OS_LIMIT		Configured limit of pending activations reached
 *
 * \see sys_task_create()
 * \see sys_task_delayed_activate()
 * \see sys_task_chain()
 */
__syscall unsigned int sys_task_activate(unsigned int task_id);

/** Activate task and let it wait on relative timeout
 *
 * A successful call to this function activates an activatable task
 * in SUSPENDED state referenced by its ID \a task_id in the caller's partition
 * after the timeout \a timeout elapsed.
 * If \a sync is non-zero, the timeout is set relative to the next partition
 * release point. Otherwise the timeout is relative to the current time.
 * After activation, the task will be in "waiting for activation" state.
 *
 * \param [in] task_id		ID of the task to activate
 * \param [in] sync			Timeout relative to partition start
 * \param [in] timeout		Relative timeout >=0
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid task ID
 * \retval E_OS_ACCESS		Task refers to an ISR or hook
 * \retval E_OS_ACCESS		Task is a non-activatable task
 * \retval E_OS_STATE		Task is already activated
 * \retval E_OS_RESOURCE	Invalid timeout, 0 or infinity
 *
 * \see sys_task_create()
 * \see sys_task_activate()
 * \see sys_task_chain()
 * \see sys_wait_periodic()
 * \see timeout_t
 */
__syscall unsigned int sys_task_delayed_activate(unsigned int task_id, int sync, timeout_t timeout);

/** Get scheduling state of a task
 *
 * A successful call to this function returns the current scheduling state
 * of task \a task_id in \a state.
 *
 * \param [in] task_id		ID of the task
 * \param [out] state		Task's state
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid task ID
 */
__syscall unsigned int sys_task_get_state(unsigned int task_id, unsigned int *state);


/** Get priority of a task
 *
 * A successful call to this function returns the current scheduling priority
 * of task \a task_id in \a prio.
 * If \a task_id refers to a task in suspended state, the returned priority
 * is undefined.
 *
 * \param [in] task_id		ID of the task
 * \param [out] prio		Task's priority
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid task ID
 *
 * \see sys_task_set_prio()
 * \see sys_fast_prio_get()
 * \see sys_fast_prio_set()
 * \see sys_fast_prio_sync()
 * \see user_error_state_t::user_prio
 */
__syscall unsigned int sys_task_get_prio(unsigned int task_id, unsigned int *prio);

/** Set priority of a task
 *
 * A successful call to this function sets the priority of task \a task_id
 * to the new scheduling priority \a prio.
 * Changes to the scheduling priority of the caller's task override any
 * settings in the user scheduling state.
 *
 * \param [in] task_id		ID of the task
 * \param [in] prio			New priority
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid task ID
 * \retval E_OS_STATE		Task is suspended
 * \retval E_OS_VALUE		Priority \a prio out of limits
 *
 * \see sys_task_get_prio()
 * \see sys_fast_prio_get()
 * \see sys_fast_prio_set()
 * \see sys_fast_prio_sync()
 * \see user_error_state_t::user_prio
 */
__syscall unsigned int sys_task_set_prio(unsigned int task_id, unsigned int prio);

/** Get priority of current task
 *
 * This service returns the user space scheduling priority of the current task
 * without system call overhead.
 * If a priority above the partition's priority limit was set with
 * sys_fast_prio_set(), the returned priority may reflect the priority above
 * the limit or a value bounded to the partition's priority limit.
 *
 * \return Scheduling priority
 *
 * \see sys_task_get_prio()
 * \see sys_task_set_prio()
 * \see sys_fast_prio_set()
 * \see sys_fast_prio_sync()
 * \see user_error_state_t::user_prio
 */
unsigned int sys_fast_prio_get(void);

/** Set priority of current task
 *
 * This service sets the user space scheduling priority of the current task
 * to \a prio and returns the previous scheduling priority
 * without system call overhead if possible.
 * If the new scheduling priority is below the current scheduling priority
 * and would cause preemption if a another task has higher priority, the
 * service issues a system call to preempt the caller.
 * The new priority \a prio will be bounded to the partition's priority limit.
 *
 * \return Previous scheduling priority
 *
 * \see sys_task_get_prio()
 * \see sys_task_set_prio()
 * \see sys_fast_prio_get()
 * \see sys_fast_prio_sync()
 * \see user_error_state_t::user_prio
 * \see user_error_state_t::next_prio
 */
unsigned int sys_fast_prio_set(unsigned int prio);

/** Synchronize user space scheduling priority
 *
 * A call to this function synchronizes the user space scheduling priority
 * with the kernel and probably preempts the caller if another task of
 * higher priority is ready to execute.
 *
 * \see sys_fast_prio_get()
 * \see sys_fast_prio_set()
 * \see user_error_state_t::user_prio
 * \see user_error_state_t::next_prio
 */
__syscall void sys_fast_prio_sync(void);


/** Yield current task at current priority
 *
 * A call to this function yields the current task without changing its
 * current scheduling priority.
 */
__syscall void sys_yield(void);

/** Yield current task
 *
 * A successful call to this function yields the current task at its configured
 * base priority, which may be lower than the current elevated priority.
 * The current scheduling priority must be the task's configured elevated
 * priority, which is also restored when the task is scheduled again.
 *
 * \retval E_OK				Success
 * \retval E_OS_RESOURCE	Task is currently not on elevated priority.
 *
 * \note This is used to implement OSEK non-preemptive scheduling
 * and releases internal resources.
 */
__syscall unsigned int sys_schedule(void);


/** Set events in target task's pending event mask
 *
 * A successful call to this function sets the events in the event mask \a mask
 * as pending events to the target task \a task_id.
 * If the target task is currently waiting on one of these events, the target
 * task is unblocked in the scheduler.
 *
 * \param [in] task_id		ID of the task
 * \param [in] mask			Mask of events to set
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid task ID
 * \retval E_OS_ACCESS		Task has no events (can not block)
 * \retval E_OS_STATE		Task is suspended
 */
__syscall unsigned int sys_ev_set(unsigned int task_id, evmask_t mask);

/** Get events from target task's pending event mask
 *
 * A successful call to this function returns the currently pending events of
 * task \a task_id in \a mask.
 *
 * \param [in] task_id		ID of the task
 * \param [out] mask		Mask of pending events
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid task ID
 * \retval E_OS_ACCESS		Task has no events (can not block)
 * \retval E_OS_STATE		Task is suspended
 */
__syscall unsigned int sys_ev_get(unsigned int task_id, evmask_t *mask);

/** Clear set of events from the current task's pending event mask
 *
 * A successful call to this function clears all events given in the event
 * mask \a mask from the current task's pending event mask.
 *
 * \param [in] mask			Mask of events to clear
 *
 * \retval E_OK				Success
 * \retval E_OS_ACCESS		Task has no events (can not block)
 */
__syscall unsigned int sys_ev_clear(evmask_t mask);

/** Waits for set of events to be set in pending event mask;
 * and clears and returns pending events in one call.
 *
 * A successful call to this function let the current task wait for
 * the events specified in \a mask to be set in the task's pending event mask.
 * If the specified events are already set in the event mask, the function
 * returns immediately without waiting.
 * When one of the specified events is or will be set in the pending event mask,
 * the function clears the events given in \a clear from the pending event mask.
 * The set of pending events before clearing is returned in \a set.
 *
 * \param [in] mask			Mask of events to wait for
 * \param [in] clear		Mask of events to clear after waiting
 * \param [out] set			Mask of pending events
 *
 * \retval E_OK				Success
 * \retval E_OS_ACCESS		Task has no events (can not block)
 */
__syscall unsigned int sys_ev_wait_get_clear(evmask_t mask, evmask_t clear, evmask_t *set);

/** Waits for set of events to be set in pending event mask
 *
 * A successful call to this function let the current task wait for
 * the events specified in \a mask to be set in the task's pending event mask.
 * If the specified events are already set in the event mask, the function
 * returns immediately without waiting.
 *
 * \param [in] mask			Mask of events to wait for
 *
 * \retval E_OK				Success
 * \retval E_OS_ACCESS		Task has no events (can not block)
 */
static inline __alwaysinline unsigned int sys_ev_wait(evmask_t mask)
{
	evmask_t _sys_ev_wait_dummy_get_mask;
	return sys_ev_wait_get_clear(mask, 0, &_sys_ev_wait_dummy_get_mask);
}

/** Set event in task's pending event mask in own or remote partition
 *
 * A successful call to this function sets a configured event in the pending
 * event mask of a configured task in a configured partition.
 * The configured event, task, and partition is identified by its
 * inter partition event ID (IPEV) \a ipev_id.
 * If the target task is currently waiting on one of these events, the target
 * task is unblocked in the scheduler.
 *
 * \param [in] ipev_id		ID of the inter partition event (IPEV)
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid IPEV ID
 *
 * \note No error is returned if the target task is in suspended state
 * or its partition is not active. The event is lost in this case.
 */
__syscall unsigned int sys_ipev_set(unsigned int ipev_id);


/** Call a kernel level device driver (KLDD)
 *
 * A successful call to this function invokes a configured kernel level
 * device driver (KLDD) identified by its ID \a kldd_id with three
 * arguments \a arg1, \a arg2, and \ arg3.
 * The meaning of the arguments depends on the KLDD.
 *
 * \param [in] kldd_id		ID of the KLDD
 * \param [in/out] arg1		General purpose argument 1
 * \param [in/out] arg2		General purpose argument 2
 * \param [in/out] arg3		General purpose argument 3
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid KLDD ID
 * \note The KLDD can return further errors.
 */
__syscall unsigned int sys_kldd_call(unsigned int kldd_id, unsigned long arg1,
                           unsigned long arg2, unsigned long arg3);


/** Get current partition ID
 *
 * A call to this function returns the ID of the current partition.
 *
 * \return Current partition ID
 */
__syscall unsigned int sys_part_self(void);


/** Get operating mode of current partition
 *
 * A call to this function returns the current partition's operating mode.
 *
 * \return Current partition's operating mode
 *
 * \see sys_part_set_operating_mode()
 * \see sys_part_get_operating_mode_ex()
 * \see sys_part_set_operating_mode_ex()
 * \see sys_part_get_start_condition()
 */
__syscall unsigned int sys_part_get_operating_mode(void);

/** Get start condition of current partition
 *
 * A call to this function returns the current partition's start condition.
 *
 * \return Current partition's start condition
 *
 * \see sys_part_set_operating_mode()
 * \see sys_part_get_operating_mode_ex()
 */
__syscall unsigned int sys_part_get_start_condition(void);

/** Change operating mode of current partition
 *
 * A successful call to this function changes the current partition's operating
 * mode to \a new_mode:
 * - The partition's current operating mode is one of COLD_START, WARM_START,
 *   or NORMAL.
 * - Setting the operating mode to IDLE results in a shutdown of the partition.
 * - Setting the operating mode to either COLD_START or WARM_START results in
 *   a restart of the partition, if possible.
 * - The transition to WARM_START requires that the partition currently
 *   is in NORMAL mode.
 * - Setting the operating mode to NORMAL from COLD_START or WARM_START releases
 *   all tasks waiting for the transition to NORMAL.
 * - The transition to NORMAL from NORMAL mode has no effect.
 * - The transition to NORMAL mode enables health monitoring.
 *
 * \param [in] new_mode		Operating mode
 *
 * \retval E_OK				Success
 * \retval E_OS_VALUE		Invalid operating mode
 * \retval E_OS_STATE		Transition to WARM_START from COLD_START
 * \retval E_OS_NOFUNC		Transition from NORMAL to NORMAL
 *
 * \see sys_part_get_operating_mode()
 * \see sys_part_get_operating_mode_ex()
 * \see sys_part_set_operating_mode_ex()
 */
__syscall unsigned int sys_part_set_operating_mode(
	unsigned int new_mode);

/** Get operating mode of partition (privileged)
 *
 * A call to this function returns the operating mode of partition \a part_id
 * in \a mode_p and its start condition in \a condition_p.
 *
 * \param [in] part_id		Partition ID
 * \param [out] mode_p		Operating mode
 * \param [out] condition_p	Start condition
 *
 * \retval E_OK				Success
 * \retval E_OS_ACCESS		Missing partition privilege
 * \retval E_OS_ID			Invalid partition ID
 *
 * \note This service requires privileges to modify other partitions.
 *
 * \see sys_part_self()
 * \see sys_part_get_operating_mode()
 * \see sys_part_set_operating_mode()
 * \see sys_part_set_operating_mode_ex()
 * \see sys_part_get_start_condition()
 */
__syscall unsigned int sys_part_get_operating_mode_ex(
	unsigned int part_id,
	unsigned int *mode_p,
	unsigned int *condition_p);

/** Change operating mode of partition (privileged)
 *
 * A successful call to this function changes the operating mode of partition
 * \a part_id to \a new_mode. If \a part_id refers to the current partition,
 * the transitions described in sys_part_set_operating_mode() apply.
 * If \a part_id refers to any other partition, the following transitions
 * are allowed:
 * - The partition's current operating mode is one of IDLE, COLD_START,
 *   WARM_START, or NORMAL.
 * - Setting the operating mode to IDLE results in a shutdown of the partition.
 * - Setting the operating mode to either COLD_START or WARM_START results in
 *   a restart of the partition, if possible.
 * - The transition to WARM_START requires that the partition is in NORMAL mode
 *   or is in IDLE mode, but was in NORMAL mode before entering IDLE mode
 * - The transition to NORMAL mode is prohibited.
 *
 * \param [in] part_id		Partition ID
 * \param [in] new_mode		Operating mode
 *
 * \retval E_OK				Success
 * \retval E_OS_ACCESS		Missing partition privilege
 * \retval E_OS_ID			Invalid partition ID
 * \retval E_OS_VALUE		Invalid operating mode
 * \retval E_OS_STATE		Transition to NORMAL for non-current partition
 * \retval E_OS_STATE		Transition to WARM_START from COLD_START
 * \retval E_OS_LIMIT		Partition mode cannot be changed
 *
 * \see sys_part_self()
 * \see sys_part_get_operating_mode()
 * \see sys_part_set_operating_mode()
 * \see sys_part_get_operating_mode_ex()
 */
__syscall unsigned int sys_part_set_operating_mode_ex(
	unsigned int part_id,
	unsigned int new_mode);


/** Increment software counter
 *
 * A successful call to this function increments the software counter
 * identified \a ctr_id by one tick. Depending on currently active alarms
 * and schedule tables, tasks may be activated.
 *
 * \param [in] ctr_id		Counter ID
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid counter ID
 * \retval E_OS_ID			Counter is a hardware counter
 *
 * \see sys_ctr_get()
 * \see sys_ctr_elapsed()
 * \see sys_alarm_base()
 */
__syscall unsigned int sys_ctr_increment(unsigned int ctr_id);

/** Get current counter value
 *
 * A successful call to this function returns the current value
 * of the counter \a ctr_id in \a value.
 * If the counter is a hardware counter, the current hardware value is returned
 * if the counter resides on the caller's CPU.
 *
 * \param [in] ctr_id		Counter ID
 * \param [out] value		Current counter value
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid counter ID
 *
 * \see sys_ctr_increment()
 * \see sys_ctr_elapsed()
 * \see sys_alarm_base()
 */
__syscall unsigned int sys_ctr_get(unsigned int ctr_id, ctrtick_t *value);

/** Get elapsed counter value in ticks
 *
 * A successful call to this function returns the current value
 * of the counter \a ctr_id in \a value and provides the elapsed time
 * since a previous value \a previous in \a elapsed.
 *
 * \param [in] ctr_id		Counter ID
 * \param [in] previous		Previous counter value
 * \param [out] value		Current counter value
 * \param [out] elapsed		Elapsed time to \a previous
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid counter ID
 * \retval E_OS_VALUE		Value of \a previous exceeds the counter range
 *
 * \see sys_ctr_increment()
 * \see sys_ctr_get()
 * \see sys_alarm_base()
 */
__syscall unsigned int sys_ctr_elapsed(unsigned int ctr_id, ctrtick_t previous, ctrtick_t *value, ctrtick_t *elapsed);

/** Get configuration of counter associated to alarm
 *
 * A successful call to this function returns the configuration attributes
 * of an alarm's associated counter. The alarm is identified by \a alarm_id.
 * The returned configuration attributes of the counter are:
 * - the maximum value of the counter in ticks in \a maxallowedvalue
 * - number of ticks of the counter to reach a significant unit in \a ticksperbase
 * - the minimum value for a cycle in \a mincycle
 *
 * \param [in] alarm_id		Alarm ID
 * \param [out] maxallowedvalue	Maximum counter value
 * \param [out] ticksperbase	Significant unit
 * \param [out] mincycle	Minimum cycle
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid alarm ID
 *
 * \see sys_ctr_increment()
 * \see sys_ctr_get()
 * \see sys_ctr_elapsed()
 * \see sys_alarm_get()
 */
__syscall unsigned int sys_alarm_base(unsigned int alarm_id, ctrtick_t *maxallowedvalue, ctrtick_t *ticksperbase, ctrtick_t *mincycle);

/** Get relative number of ticks before alarm expires
 *
 * A successful call to this function returns the number of ticks
 * before the alarm \a alarm_id expires in \a value.
 *
 * \param [in] alarm_id		Alarm ID
 * \param [out] value		Relative number of ticks before alarm expires
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid alarm ID
 * \retval E_OS_NOFUNC		Alarm is not active
 *
 * \see sys_ctr_increment()
 * \see sys_ctr_get()
 * \see sys_ctr_elapsed()
 * \see sys_alarm_base()
 */
__syscall unsigned int sys_alarm_get(unsigned int alarm_id, ctrtick_t *value);

/** Set relative number of ticks before alarm expires
 *
 * A successful call to this function activates the alarm \a alarm_id
 * to expire at \a increment counter ticks relative to the current
 * counter value.
 * If \a cycle is non-zero, the alarm repeats periodically each \a cycle
 * ticks after the first alarm expiration.
 *
 * \param [in] alarm_id		Alarm ID
 * \param [in] increment	Relative number of ticks before alarm expires
 * \param [in] cycle		Alarm cycle, relative to expiry if non-zero
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid alarm ID
 * \retval E_OS_NOFUNC		Alarm is already active
 * \retval E_OS_VALUE		\a increment is zero or exceeds the counter range
 * \retval E_OS_VALUE		\a cycle is set, but smaller than minimum cycle
 * \retval E_OS_VALUE		\a cycle is set and exceeds the counter range
 *
 * \see sys_alarm_set_abs()
 * \see sys_alarm_cancel()
 */
__syscall unsigned int sys_alarm_set_rel(unsigned int alarm_id, ctrtick_t increment, ctrtick_t cycle);

/** Set absolute number of ticks where alarm expires
 *
 * A successful call to this function activates the alarm \a alarm_id
 * to expire at the absolute time \a expiry of counter ticks.
 * If \a cycle is non-zero, the alarm repeats periodically each \a cycle
 * ticks after the first alarm expiration.
 *
 * \param [in] alarm_id		Alarm ID
 * \param [in] expiry		Absolute tick value of alarm expiration
 * \param [in] cycle		Alarm cycle, relative to expiry if non-zero
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid alarm ID
 * \retval E_OS_NOFUNC		Alarm is already active
 * \retval E_OS_VALUE		\a expiry exceeds the counter range
 * \retval E_OS_VALUE		\a cycle is set, but smaller than minimum cycle
 * \retval E_OS_VALUE		\a cycle is set and exceeds the counter range
 *
 * \see sys_alarm_set_rel()
 * \see sys_alarm_cancel()
 */
__syscall unsigned int sys_alarm_set_abs(unsigned int alarm_id, ctrtick_t expiry, ctrtick_t cycle);

/** Cancel alarm
 *
 * A successful call to this function cancels the active alarm \a alarm_id
 * before it expires.
 *
 * \param [in] alarm_id		Alarm ID
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid alarm ID
 * \retval E_OS_NOFUNC		Alarm is not active
 *
 * \see sys_alarm_set_rel()
 * \see sys_alarm_set_abs()
 */
__syscall unsigned int sys_alarm_cancel(unsigned int alarm_id);


/** Start scheduling table at relative counter value
 *
 * A successful call to this function starts a schedule table \a schedtab_id
 * with a synchronization strategy of NONE or EXPLICIT at \a offset counter
 * ticks relative to the current counter value.
 * The schedule table's state changes from STOPPED to RUNNING.
 *
 * \param [in] schedtab_id	ID of the scheduling table
 * \param [in] offset		Relative counter offset
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid schedule table ID
 * \retval E_OS_ID			Schedule table uses IMPLICIT synchronization strategy
 * \retval E_OS_STATE		Schedule table is not in STOPPED state
 * \retval E_OS_VALUE		\a offset is zero or exceeds the counter range
 *
 * \see sys_schedtab_start_abs()
 * \see sys_schedtab_stop()
 * \see sys_schedtab_start_sync()
 */
__syscall unsigned int sys_schedtab_start_rel(
	unsigned int schedtab_id,
	ctrtick_t offset);

/** Start scheduling table at absolute counter value
 *
 * A successful call to this function starts a schedule table \a schedtab_id
 * of any synchronization strategy when the counter reaches the value \a start.
 * The schedule table's state changes from STOPPED to RUNNING or RUNNING_SYNC.
 *
 * \param [in] schedtab_id	ID of the scheduling table
 * \param [in] start		Absolute counter value
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid schedule table ID
 * \retval E_OS_STATE		Schedule table is not in STOPPED state
 * \retval E_OS_VALUE		\a start exceeds the counter range
 *
 * \see sys_schedtab_start_rel()
 * \see sys_schedtab_stop()
 * \see sys_schedtab_start_sync()
 */
__syscall unsigned int sys_schedtab_start_abs(
	unsigned int schedtab_id,
	ctrtick_t start);

/** Stop scheduling table
 *
 * A successful call to this function stops the running or waiting
 * schedule table \a schedtab_id.
 * The schedule table's state changes from NEXT, WAITING, RUNNING,
 * RUNNING_SYNC, or RUNNING_ASYNC to STOPPED.
 *
 * \param [in] schedtab_id	ID of the scheduling table
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid schedule table ID
 * \retval E_OS_NOFUNC		Schedule table is already stopped
 *
 * \see sys_schedtab_start_rel()
 * \see sys_schedtab_start_abs()
 * \see sys_schedtab_start_sync()
 */
__syscall unsigned int sys_schedtab_stop(
	unsigned int schedtab_id);

/** Set successor of scheduling table
 *
 * A successful call to this function sets the successor of the schedule table
 * \a schedtab_id_from to schedule table \a schedtab_id_to.
 * When the first schedule table completes, it will enter STOPPED state.
 * The successor schedule table's state changes from STOPPED to NEXT, and later
 * to a running state when the first schedule table stops.
 *
 * \param [in] schedtab_id_from	ID of the scheduling table to replace
 * \param [in] schedtab_id_to	ID of the scheduling table as next
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid schedule table ID
 * \retval E_OS_ID			Schedule tables are driven by different counters
 * \retval E_OS_ID			Schedule tables have different synchronization strategy
 * \retval E_OS_NOFUNC		Originating schedule table is not started
 * \retval E_OS_STATE		Successing schedule table is not stopped
 *
 * \see sys_schedtab_stop()
 */
__syscall unsigned int sys_schedtab_next(
	unsigned int schedtab_id_from,
	unsigned int schedtab_id_to);

/** Start explicitly synchronized scheduling table synchronously
 *
 * A call to this function starts a schedule table
 * with EXPLICIT synchronization strategy.
 * The schedule table's state changes from STOPPED to WAITING.
 *
 * \param [in] schedtab_id	ID of the scheduling table
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid schedule table ID
 * \retval E_OS_ID			Schedule table does not use EXPLICIT synchronization strategy
 * \retval E_OS_STATE		Schedule table is not in STOPPED state
 *
 * \see sys_schedtab_start_rel()
 * \see sys_schedtab_start_abs()
 * \see sys_schedtab_stop()
 */
__syscall unsigned int sys_schedtab_start_sync(
	unsigned int schedtab_id);

/** Provide synchronization count to scheduling table
 *
 * A call to this function provides a synchronization count of \a value
 * to schedule table \a schedtab_id.
 * If the schedule table is in state WAITING, it enters RUNNING_SYNC state.
 * Otherwise the running schedule table's synchronization is set to
 * either RUNNING_SYNC or RUNNING_ASYNC, depending on the counter deviation.
 *
 * \param [in] schedtab_id	ID of the scheduling table
 * \param [in] value		Absolute counter value
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid schedule table ID
 * \retval E_OS_ID			Schedule table does not use EXPLICIT synchronization strategy
 * \retval E_OS_STATE		Schedule table is not running or in WAITING state
 * \retval E_OS_VALUE		\a value exceeds the counter range
 *
 * \see sys_schedtab_start_sync()
 * \see sys_schedtab_set_async()
 */
__syscall unsigned int sys_schedtab_sync(
	unsigned int schedtab_id,
	ctrtick_t value);

/** Stop explicit synchronization of scheduling table
 *
 * A call to this function stops explicit synchronization of schedule table
 * \a schedtab_id.
 * The schedule table's state changes from RUNNING_SYNC or RUNNING_ASYNC
 * to RUNNING.
 *
 * \param [in] schedtab_id	ID of the scheduling table
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid schedule table ID
 * \retval E_OS_ID			Schedule table does not use EXPLICIT synchronization strategy
 * \retval E_OS_STATE		Schedule table is not running
 *
 * \see sys_schedtab_start_sync()
 * \see sys_schedtab_sync()
 */
__syscall unsigned int sys_schedtab_set_async(
	unsigned int schedtab_id);

/** Get scheduling table state
 *
 * A call to this function returns the state of scheduling table \a schedtab_id
 * in \a state.
 *
 * \param [in] schedtab_id	ID of the scheduling table
 * \param [out] state		Scheduling table's state
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid schedule table ID
 */
__syscall unsigned int sys_schedtab_get_state(
	unsigned int schedtab_id,
	unsigned int *state);


/** Set wait queue discipline
 *
 * For the wait queue indentified by \a wq_id, this call sets the wait queue
 * discipline (FIFO or PRIORITY) according to \a discipline and the wait queue
 * becomes eligible for waiting.
 * The wait queue discipline can be changed afterwards as long as no
 * tasks are waiting.
 * This call also registers the associated state variable in user space
 * \a user_state to the wait queue.
 *
 * \param [in] wq_id		ID of the wait queue
 * \param [in] discipline	Wait discipline, FIFO or PRIORITY
 * \param [in] user_state	State variable
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid wait queue ID
 * \retval E_OS_NOFUNC		Wait queue is not connected
 * \retval E_OS_STATE		Wait queue already in use (tasks are waiting)
 * \retval E_OS_VALUE		Invalid discipline
 * \retval E_OS_ILLEGAL_ADDRESS	Address of state variable invalid
 *
 * \see sys_wq_wait()
 * \see sys_wq_wake()
 */
__syscall unsigned int sys_wq_set_discipline(
	unsigned int wq_id,
	unsigned int discipline,
	uint32_t *user_state);

/** Wait on wait queue
 *
 * On a successful call to this function, the calling thread waits on
 * the wait queue \a wq_id until the specified timeout \a timeout expires,
 * the task is woken up by a call to sys_wq_wait(), or waiting is cancelled
 * by a call to sys_unblock().
 * For the wait queue uses the PRIORITY discipline, waiting tasks are ordered
 * by their waiting priority \a prio. The waiting priority is ignored
 * in the FIFO discipline.
 * Before letting the task enter the wait state, the kernel compares
 * the associated state variable in user space with \a compare. If not equal,
 * waiting prevented.
 * A timeout of 0 returns immediately.
 *
 * \note The wait queue needs to be initialized with a call
 * to sys_wq_set_discipline() before tasks can wait.
 *
 * \param [in] wq_id		ID of the wait queue
 * \param [in] compare		Compare value for state variable
 * \param [in] timeout		Relative timeout, including 0 and infinity
 * \param [in] prio			Queuing priority
 *
 * \retval E_OK				Successfully woken up by sys_wq_wake()
 * \retval E_OS_ID			Invalid wait queue ID
 * \retval E_OS_NOFUNC		Wait queue is not initialized (no discipline set)
 * \retval E_OS_VALUE		Comparation failed
 * \retval E_OS_ACCESS		Task is not allowed to block
 * \retval E_OS_TIMEOUT		Timeout expired
 * \retval E_OS_ILLEGAL_ADDRESS	Invalid pointer
 * \retval E_OS_STATE		Woken up by sys_unblock()
 *
 * \see sys_wq_set_discipline()
 * \see sys_wq_wake()
 * \see sys_unblock()
 * \see timeout_t
 */
static inline __alwaysinline unsigned int sys_wq_wait(
	unsigned int wq_id,
	uint32_t compare,
	timeout_t timeout,
	unsigned int prio)
{
	/* NOTE: internal wrapper for sys_wq_wait: the kernel's syscall interface
	 * doesn't uses parameters on stack and the Tricore ABI limits us to 4 data
	 * and 4 address arguments at the same time.
	 */
	__syscall unsigned int __sys_wq_wait(unsigned int, uint32_t, timeout_t,
	                                     void *);

	return __sys_wq_wait(wq_id, compare, timeout, (void *)prio);
}

/** Wake up tasks waiting on wait queue
 *
 * A successful call to this function wakes up to \a count tasks up waiting
 * on the wait queue \a wq_id.
 * The call succeeds, even if less tasks are currently waiting.
 *
 * \param [in] wq_id		ID of the wait queue (partition specific)
 * \param [in] count		Number of tasks to wake
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid wait queue ID
 * \retval E_OS_NOFUNC		Wait queue is not connected
 *
 * \see sys_wq_set_discipline()
 * \see sys_wq_wait()
 * \see sys_unblock()
 */
__syscall unsigned int sys_wq_wake(
	unsigned int wq_id,
	unsigned int count);

/** Wake up waiting task
 *
 * A call to this function wakes up a task \a task_id currently:
 * - waiting on a wait queue,
 * - waiting to send an RPC, or
 * - sleeping with timeout.
 *
 * \note The woken up task returns E_OS_STATE.
 *
 * \param [in] task_id		ID of the task to wakeup
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid task ID
 * \retval E_OS_NOFUNC		Task not waiting
 *
 * \see sys_wq_wait()
 * \see sys_wq_wake()
 * \see sys_sleep()
 * \see sys_rpc_call()
 * \see sys_rpc_reply()
 */
__syscall unsigned int sys_unblock(unsigned int task_id);


/** Get current system time
 *
 * A call to this function returns the current system time in nanoseconds.
 * Time is assumed to start at 0 when the system boots.
 *
 * \returns Current time in nanoseconds since boot
 *
 * \see time_t
 */
__syscall time_t sys_gettime(void);

/** Sleep until timeout expires
 *
 * A call to this function lets the current task sleep until
 * the timeout \a timeout expires or the task is woken up by sys_unblock().
 * A timeout of 0 returns immediately.
 *
 * \param [in] timeout		Relative timeout, including 0 or infinity
 *
 * \retval E_OS_TIMEOUT		Timeout expired
 * \retval E_OS_ACCESS		Task is not allowed to block
 * \retval E_OS_STATE		Woken up by sys_unblock()
 *
 * \see sys_unblock()
 * \see timeout_t
 */
__syscall unsigned int sys_sleep(timeout_t timeout);

/** Replenish deadline
 *
 * A call to this function sets the deadline expiry time of the calling task
 * to the current time plus the given \a budget.
 * If \a budget is infinity, the deadline is effectively disabled.
 *
 * \note This function has no effect when the task had no deadline before.
 *
 * \param [in] budget		Relative deadline expiry time or infinity
 *
 * \retval E_OK				Success
 * \retval E_OS_RESOURCE	Deadline exceeds period of a periodic task
 *
 * \see timeout_t
 */
__syscall unsigned int sys_replenish(timeout_t budget);

/** Wait until next partition release point
 *
 * A call to this function let the calling periodic task wait until
 * the next partition release point, typically the next partition activation.
 * The deadline of the calling task is assumed to be met and the deadline
 * expiry is deactivated.
 *
 * \retval E_OK				Woken up
 * \retval E_OS_RESOURCE	Calling task is not a periodic task
 *
 * \see sys_task_delayed_activate()
 */
__syscall unsigned int sys_wait_periodic(void);


/** Retrieve shared memory attributes
 *
 * For the shared memory \a shm_id, a successful call to this function
 * returns the start address of the shared memory in \a base_p
 * and its size in \a size_p.
 *
 * \param [in] shm_id		ID of the shared memory
 * \param [out] base_p		Start address of the shared memory
 * \param [out] size_p		Size of the shared memory
 *
 * \retval E_OK				Woken up
 * \retval E_OS_ID			Invalid shared memory ID.
 */
__syscall unsigned int sys_shm_iterate(
	unsigned int shm_id,
	addr_t *base_p,
	size_t *size_p);


/** Change time partition schedule on target CPU (privileged)
 *
 * A successful call to this function changes the time partition schedule
 * on CPU \a cpu_id to the new time partition schedule \a schedule_id.
 * The actual switch of the time partition schedule happens if the currently
 * running time partition schedule completes its last time partition window.

 * \param [in] cpu_id		ID of the target processor
 * \param [in] schedule_id	ID of the time partition schedule
 *
 * \retval E_OK				Success
 * \retval E_OS_ACCESS		Missing partition privilege
 * \retval E_OS_ID			Invalid CPU ID
 * \retval E_OS_ID			Invalid time partition schedule ID
 */
__syscall unsigned int sys_schedule_change(
	unsigned int cpu_id,
	unsigned int schedule_id);


/** Change system HM table (privileged)
 *
 * A call to this function changes the system HM (health monitoring) table
 * to \a table_id.
 *
 * \note This call is restricted to tasks executing on the first processor.
 *
 * \param [in] table_id		ID of the system HM table
 * \param [in] schedule_id	ID of the time partition schedule
 *
 * \retval E_OK				Success
 * \retval E_OS_ACCESS		Missing partition privilege
 * \retval E_OS_CORE		Called on other than first processor
 * \retval E_OS_ID			Invalid system HM table ID
 */
__syscall unsigned int sys_hm_change(
	unsigned int table_id);

/** Inject application error
 *
 * A successful call to this function injects the health monitor error
 * \a hm_error_id with extra information \a extra into HM processing.
 * Injectable errors are restricted to user errors.
 * If configured, the partition error hook will be notified with the error.
 *
 * \note If the partition error record area overflows, the error will be
 * promoted to a partition error.
 *
 * \param [in] hm_error_id	HM error ID
 * \param [in] extra		Extra information
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid health monitor error ID
 */
__syscall unsigned int sys_hm_inject(unsigned int hm_error_id, unsigned long extra);

/** Log application error message
 *
 * Log an application error message in health monitoring.
 *
 * \param [in] err_msg		Error message data
 * \param [in] size			Error message size
 *
 * \retval E_OK				Success
 * \retval E_OS_ILLEGAL_ADDRESS	Invalid pointer
 * \retval E_OS_VALUE		Invalid message size
 */
__syscall unsigned int sys_hm_log(void *err_msg, size_t size);


/** Disable interrupt source of ISR
 *
 * For the interrupt source associated with the ISR \a isr_id,
 * a call to this function disables (masks) the associated interrupt source.
 *
 * \note The target ISR cannot call this function to mask itself
 * while it is currently running.
 *
 * \param [in] isr_id		Task ID of the ISR
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid task ID
 * \retval E_OS_ACCESS		ID does not refer to an ISR task
 * \retval E_OS_STATE		Called from target ISR
 *
 * \see sys_isr_unmask()
 */
__syscall unsigned int sys_isr_mask(
	unsigned int isr_id);

/** Enable interrupt source of ISR
 *
 * For the interrupt source associated with the ISR \a isr_id,
 * a call to this function enables (unmasks) the associated interrupt source.
 *
 * \note The target ISR cannot call this function to unmask itself
 * while it is currently running.
 *
 * \param [in] isr_id		Task ID of the ISR
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid task ID
 * \retval E_OS_ACCESS		ID does not refer to an ISR task
 * \retval E_OS_STATE		Called from target ISR
 *
 * \see sys_isr_mask()
 */
__syscall unsigned int sys_isr_unmask(
	unsigned int isr_id);


/** Shutdown or reset system (privileged)
 *
 * A successful call to this function halts, resets, or shuts down the system.
 *
 * \param [in] mode			Halt mode
 *
 * \retval E_OS_ACCESS		Missing partition privilege
 *
 * \note On success, this call does not return to the caller.
 */
__syscall unsigned int sys_shutdown(haltmode_t mode);


/** Invoke a remote procedure call
 *
 * A successful call to this function activates the hook associated to the RPC
 * identified by \a rpc_id. The argument \a send_arg is passed over to the
 * activated hook. On RPC reply, the argument provided by sys_rpc_reply()
 * is returned in \a recv_arg.
 * The caller waits on the send queue of the RPC \a rpc_id until the specified
 * timeout \a timeout expires, or the task is woken up by sys_unblock().
 * When using a timeout of zero, the call succeeds only if the RPC hook
 * is in suspended state.
 * After sending, the caller always waits on the receive queue of the RPC with
 * infinite timeout.
 *
 * \param [in] rpc_id		ID of the RPC
 * \param [in] send_arg		Outgoing argument passed to the activated RPC hook
 * \param [in] timeout		Relative timeout, including 0 and infinity
 * \param [out] recv_arg	Incoming argument set on RPC reply
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid RPC ID
 * \retval E_OS_LIMIT		Limit of pending RPC calls reached
 * \retval E_OS_STATE		Activated RPC hook was terminated,
 * 							or woken up by sys_unblock()
 * \retval E_OS_TIMEOUT		Timeout expired
 *
 * \see sys_rpc_reply()
 * \see sys_unblock()
 * \see timeout_t
 *
 * \note The first argument of the activated RPC hook is set to a RPC reply ID
 * for sys_rpc_reply(); the second argument is \a send_arg.
 * Any argument set in the configuration is overwritten.
 */
__syscall unsigned int sys_rpc_call(
	unsigned int rpc_id,
	unsigned long send_arg,
	timeout_t timeout,
	unsigned long *recv_arg);

/** Reply a remote procedure call
 *
 * A successful call to this function replies the calling task or hook
 * identified by \a reply_id (first argument passed to the activated RPC hook)
 * and wakes the caller of the RPC.
 * The value \a reply_arg is handed over to the caller of the RPC.
 * \n
 * Note that any task, hook, or ISR in the same partition as the receiving
 * RPC hook can reply a message.
 * \n
 * If \a terminate is set to a non-zero value, the currently executing
 * task or hook terminates after replying the message.
 * If the RPC hook is terminated, it becomes eligible
 * to receive further pending RPC messages.
 *
 * \param [in] reply_id		ID of the RPC caller
 * \param [in] reply_arg	Argument to return to the caller of the RPC
 * \param [in] terminate	Terminate current calling task, hook, or ISR
 *
 * \retval E_OK				Success
 * \retval E_OS_ID			Invalid reply ID
 *
 * \see sys_rpc_call()
 * \see sys_unblock()
 *
 * \note On success, this call does not return to the caller
 * if \a terminate is set to a non-zero value.
 *
 * \note The \a reply_id becomes invalid if the caller was woken
 * by sys_unblock() or if the caller's partition was shut down or restarted.
 */
__syscall unsigned int sys_rpc_reply(
	unsigned int reply_id,
	unsigned long reply_arg,
	int terminate);

#endif

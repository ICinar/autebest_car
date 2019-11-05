/*
 * os_private.h
 *
 * OSEK runtime private header.
 *
 * azuepke, 2014-05-04: initial
 */

#ifndef __OS_PRIVATE_H__
#define __OS_PRIVATE_H__

#include <Os_Types.h>
#include <hv_types.h>
#include <Os_PrivateCfg.h>

/* DisableAllInterrupts.c */
/** store the previous priority before disabling interrupts */
extern uint8 __OsPrioBeforeDisableAllInterrupts;

/* SuspendAllInterrupts.c */
/** store the previous priority before disabling interrupts */
extern uint8 __OsPrioBeforeSuspendAllInterrupts;
extern uint8 __OsSuspendAllInterruptsNesting;

/* SuspendOSInterrupts.c */
/** store the previous priority before disabling interrupts */
extern uint8 __OsPrioBeforeSuspendOSInterrupts;
extern uint8 __OsSuspendOSInterruptsNesting;

/* StartOS.c */
/** current application mode set by StartOS */
extern AppModeType __OsApplicationMode;

/* _start.c, symbol used by hypervisor tooling */
extern void _start(void);

#define OSEK_STACKFILL  0xA070BE57UL

/** exception data (accessed by kernel) */
extern user_exception_state_t _os_exception_state;

/** raise an error to ErrorHook(), returns the last error passed in */
extern StatusType _OsRaiseError(
	StatusType error_code,
	OSErrorRecordType *record,
	OSServiceIdType service_id);

/** Exception Hook (invoked on exceptions) */
void _OsExceptionHook(void *arg);

/** Async Error Hook (invoked on async errors) */
void _OsAsyncErrorHook(void *arg);

/** error data with array size */
extern const uint8 _os_num_errors;
extern user_error_state_t _os_error_state[];


/** user scheduling state */
extern user_sched_state_t __sys_sched_state;

/** Helper to check if the caller is a task, but not an ISR or hook */
static inline int _OsCallerIsTask(void)
{
	return __sys_sched_state.taskid < Os_firstISRID;
}

/** Helper to check if the caller is a task or an ISR, but not a hook */
static inline int _OsCallerIsTaskOrIsr(void)
{
	return __sys_sched_state.taskid < Os_firstHookID;
}

/** Helper to check if the calling task or ISR has disabled interrupts */
static inline int _OsSomeoneDisabledInterrupts(void)
{
	return __OsSuspendAllInterruptsNesting + __OsSuspendOSInterruptsNesting;
}

/** Helper to check if the calling task or ISR has locked resources */
static inline int _OsCallerLockedResources(void)
{
	return Os_lastResTaken[__sys_sched_state.taskid] != INVALID_RESOURCE;
}

/** Enable all interrupts again (used only in error handling) */
static inline void _OsCleanupInterruptLockState(void)
{
	/* no checks: when we're running, we either have the interrupt locks, so
	 * resetting nesting counters is fine, or nobody else has, so they're
	 * already at 0. */
	__OsSuspendAllInterruptsNesting = 0;
	__OsSuspendOSInterruptsNesting = 0;
}

/** Unlock all resources a task locked (used only in error handling) */
extern void _OsCleanupResourceLockState(unsigned int task_id);

/** Terminator if task returns without calling TerminateTask() or ChainTask() */
extern void _OsTaskCleanup(void);

/** Terminator when ISR returns */
extern void _OsIsrCleanup(void);

#endif

/*
 * _OsIsrCleanup.c
 *
 * OSEK OS ISR return cleanup / error handling.
 *
 * azuepke, 2015-03-02: initial
 */

#include <hv.h>
#include <stddef.h>
#include "os_private.h"
#include <Os_Api.h>

/** common code to call the error hook */
static void _OsRaiseErrorFromIsrCleanup(StatusType error_code)
{
	OSErrorRecordType record;

	record.error_code = error_code;
	record.service_id = OSServiceId_INVALID;
	record.task_id = __sys_sched_state.taskid;

	/* raise prio */
	__sys_sched_state.user_prio = 255;

	/* prevent recursive invocation of the error hook */
	if (__OsErrorRecord == NULL) {
		__OsErrorRecord = &record;

		ErrorHook(record.error_code);

		/* FIXME: SWS_Os_00246: call the application specific error hook next */
		/* FIXME: SWS_Os_00085: call app error hook with application rights */

		__OsErrorRecord = NULL;
	}

	/* no restore of previous priority -- expects to terminate soon */
}

/** Terminator when ISR returns */
void _OsIsrCleanup(void)
{
	/* SWS_Os_00368: ISR returned with interrupts disabled */
	if (unlikely(_OsSomeoneDisabledInterrupts())) {
		_OsRaiseErrorFromIsrCleanup(E_OS_DISABLEDINT);
		_OsCleanupInterruptLockState();
	}

	/* SWS_Os_00369: ISR returned with resources locked */
	if (unlikely(_OsCallerLockedResources())) {
		_OsRaiseErrorFromIsrCleanup(E_OS_RESOURCE);
		_OsCleanupResourceLockState(__sys_sched_state.taskid);
	}

	/* ISRs don't need to call TerminateTask() */
	/* terminates immediately by caller */
}

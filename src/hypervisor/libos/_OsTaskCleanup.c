/*
 * _OsTaskCleanup.c
 *
 * OSEK OS task return cleanup / error handling.
 *
 * azuepke, 2015-02-27: initial
 */

#include <hv.h>
#include <stddef.h>
#include "os_private.h"
#include <Os_Api.h>

/** Terminator if task returns without calling TerminateTask() or ChainTask() */
void _OsTaskCleanup(void)
{
	OSErrorRecordType record;

	/* SWS_Os_00239: enable interrupts if necessary */
	if (unlikely(_OsSomeoneDisabledInterrupts())) {
		_OsCleanupInterruptLockState();
	}

	/* SWS_Os_00070: unlock resources if necessary */
	if (unlikely(_OsCallerLockedResources())) {
		_OsCleanupResourceLockState(__sys_sched_state.taskid);
	}

	/* SWS_Os_00069: setup error record for the missing end */
	record.error_code = E_OS_MISSINGEND;
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

	/* no restore of previous priority -- expects to terminate immediately */
}

/*
 * TerminateTask.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-05-04: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType TerminateTask(void)
{
	OSErrorRecordType record;
	StatusType err;

	if (unlikely(!_OsCallerIsTask())) {
		err = E_OS_CALLEVEL;
		goto error;
	}

#if 1
	/* SWS_Os_FIXME: terminate task with interrupts disabled */
	/* FIXME: not specified in AUTOSAR, but analoguous to SWS_Os_00368 */
	if (unlikely(_OsSomeoneDisabledInterrupts())) {
		err = E_OS_DISABLEDINT;
		goto error;
	}
#endif

	if (unlikely(_OsCallerLockedResources())) {
		err = E_OS_RESOURCE;
error:
		return _OsRaiseError(err, &record, OSServiceId_TerminateTask);
	}

	sys_task_terminate();
	unreachable();
}

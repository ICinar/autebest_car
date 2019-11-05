/*
 * Schedule.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-05-04: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType Schedule(void)
{
	OSErrorRecordType record;
	StatusType err;

	if (unlikely(!_OsCallerIsTask())) {
		err = E_OS_CALLEVEL;
		goto error;
	}

	/* NOTE: the kernel checks internally if the task holds resources
	 * by checking if the task's priority is its original priority.
	 * In this case, the kernel returns E_OS_RESOURCE.
	 */
	err = sys_schedule();
	if (unlikely(err != E_OK)) {
error:
		return _OsRaiseError(err, &record, OSServiceId_Schedule);
	}

	return err;
}

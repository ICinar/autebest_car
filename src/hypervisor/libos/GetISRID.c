/*
 * GetISRID.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-05-04: initial
 * azuepke, 2015-03-02: make usable in error handling
 */

#include <Os_Api.h>
#include <hv.h>
#include <stddef.h>
#include "os_private.h"

ISRType GetISRID(void)
{
	ISRType IsrID;

	IsrID = __sys_sched_state.taskid;
	if (__OsErrorRecord != NULL) {
		/* override ID when in error handling */
		IsrID = __OsErrorRecord->task_id;
	}

	if ((IsrID < Os_firstISRID) || (IsrID >= Os_firstHookID)) {
		/* ID refers to a normal task or hook */
		IsrID = INVALID_ISR;
	}

	return IsrID;
}

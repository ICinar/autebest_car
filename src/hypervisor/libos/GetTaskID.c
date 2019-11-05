/*
 * GetTaskID.c
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

StatusType GetTaskID(TaskRefType TaskIDRef)
{
	TaskType TaskID;

	TaskID = __sys_sched_state.taskid;
	if (__OsErrorRecord != NULL) {
		/* override ID when in error handling */
		TaskID = __OsErrorRecord->task_id;
	}

	if (TaskID >= Os_firstISRID) {
		/* ID refers to an ISR or hook */
		/* get the last valid task ID from the kernel (or INVALID_TASK) */
		TaskID = sys_task_current();
	}

	*TaskIDRef = TaskID;

	return E_OK;
}

/*
 * GetTaskState.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-05-04: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType GetTaskState(TaskType TaskID, TaskStateRefType StateRef)
{
	OSErrorRecordType record;
	unsigned int state;
	StatusType err;

	record.arg1 = TaskID;
	record.arg2 = (unsigned long)StateRef;

	if (unlikely(TaskID >= Os_firstISRID)) {
		err = E_OS_ID;
		goto error;
	}

	err = sys_task_get_state(TaskID, &state);
	if (unlikely(err != E_OK)) {
error:
		return _OsRaiseError(err, &record, OSServiceId_GetTaskState);
	}

	/* NOTE: in case of an error, *StateRef is not updated */

	switch (state) {
	case TASK_STATE_RUNNING:
		*StateRef = RUNNING;
		break;

	case TASK_STATE_READY:
		*StateRef = READY;
		break;

	case TASK_STATE_SUSPENDED:
		*StateRef = SUSPENDED;
		break;

	default:
		/* NOTE: this covers all waiting states of the kernel, e.g.
		 * TASK_STATE_WAIT_EV, TASK_STATE_WAIT_WQ, TASK_STATE_WAIT_ACT,
		 * TASK_STATE_WAIT_SEND, and TASK_STATE_WAIT_SEND.
		 */
		*StateRef = WAITING;
		break;
	}

	return E_OK;
}

/*
 * ActivateTask.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-05-04: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType ActivateTask(TaskType TaskID)
{
	OSErrorRecordType record;
	StatusType err;

	record.arg1 = TaskID;

	err = sys_task_activate(TaskID);
	if (unlikely(err != E_OK)) {
		return _OsRaiseError(err, &record, OSServiceId_ActivateTask);
	}

	return err;
}

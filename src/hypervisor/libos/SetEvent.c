/*
 * SetEvent.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-05-04: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType SetEvent(TaskType TaskID, EventMaskType Mask)
{
	OSErrorRecordType record;
	StatusType err;

	record.arg1 = TaskID;
	record.arg2 = Mask;

	err = sys_ev_set(TaskID, Mask);
	if (unlikely(err != E_OK)) {
		return _OsRaiseError(err, &record, OSServiceId_SetEvent);
	}

	return err;
}

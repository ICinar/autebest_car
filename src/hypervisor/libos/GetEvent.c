/*
 * GetEvent.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-05-04: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType GetEvent(TaskType TaskID, EventMaskRefType MaskRef)
{
	OSErrorRecordType record;
	StatusType err;

	record.arg1 = TaskID;
	record.arg2 = (unsigned long)MaskRef;

	/* NOTE: the kernel defines the mask as uint, but AUTOSAR requires ulong */
	err = sys_ev_get(TaskID, (evmask_t*)MaskRef);
	if (unlikely(err != E_OK)) {
		return _OsRaiseError(err, &record, OSServiceId_GetEvent);
	}

	return err;
}

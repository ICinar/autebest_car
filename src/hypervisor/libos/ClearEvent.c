/*
 * ClearEvent.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-05-04: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType ClearEvent(EventMaskType Mask)
{
	OSErrorRecordType record;
	StatusType err;

	record.arg1 = Mask;

	if (unlikely(!_OsCallerIsTask())) {
		err = E_OS_CALLEVEL;
		goto error;
	}

	err = sys_ev_clear(Mask);
	if (unlikely(err != E_OK)) {
error:
		return _OsRaiseError(err, &record, OSServiceId_ClearEvent);
	}

	return err;
}

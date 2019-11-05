/*
 * WaitGetClearEvent.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2015-05-12: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType WaitGetClearEvent(EventMaskType WaitMask, EventMaskType ClearMask, EventMaskRefType MaskRef)
{
	OSErrorRecordType record;
	StatusType err;

	record.arg1 = WaitMask;
	record.arg2 = ClearMask;
	record.arg3 = (unsigned long)MaskRef;

	if (unlikely(!_OsCallerIsTask())) {
		err = E_OS_CALLEVEL;
		goto error;
	}

#if 1
	/* SWS_Os_FIXME: wait on event with interrupts disabled */
	/* FIXME: not specified in AUTOSAR, but analoguous to SWS_Os_00622 */
	if (unlikely(_OsSomeoneDisabledInterrupts())) {
		err = E_OS_DISABLEDINT;
		goto error;
	}
#endif

	/* FIXME: calling WaitEvent() with RES_SCHEDULER is considered OK! */
	if (unlikely(_OsCallerLockedResources())) {
		err = E_OS_RESOURCE;
		goto error;
	}

	/* NOTE: kernel return an error if task is not an extended task */
	err = sys_ev_wait_get_clear(WaitMask, ClearMask, (evmask_t*)MaskRef);
	if (unlikely(err != E_OK)) {
error:
		return _OsRaiseError(err, &record, OSServiceId_WaitGetClearEvent);
	}

	return err;
}

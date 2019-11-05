/*
 * GetElapsedValue.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-08-07: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType GetElapsedValue(CounterType CounterID, TickRefType ValueRef, TickRefType ElapsedValueRef)
{
	OSErrorRecordType record;
	StatusType err;

	record.arg1 = CounterID;
	record.arg2 = (unsigned long)ValueRef;
	record.arg3 = (unsigned long)ElapsedValueRef;

	err = sys_ctr_elapsed(CounterID, *(ctrtick_t *)ElapsedValueRef, (ctrtick_t *)ValueRef, (ctrtick_t *)ElapsedValueRef);
	if (unlikely(err != E_OK)) {
		return _OsRaiseError(err, &record, OSServiceId_GetElapsedValue);
	}

	return err;
}

/*
 * GetCounterValue.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-08-07: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType GetCounterValue(CounterType CounterID, TickRefType ValueRef)
{
	OSErrorRecordType record;
	StatusType err;

	record.arg1 = CounterID;
	record.arg2 = (unsigned long)ValueRef;

	err = sys_ctr_get(CounterID, (ctrtick_t *)ValueRef);
	if (unlikely(err != E_OK)) {
		return _OsRaiseError(err, &record, OSServiceId_GetCounterValue);
	}

	return err;
}

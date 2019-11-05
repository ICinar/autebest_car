/*
 * IncrementCounter.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-08-07: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType IncrementCounter(CounterType CounterID)
{
	OSErrorRecordType record;
	StatusType err;

	record.arg1 = CounterID;

	err = sys_ctr_increment(CounterID);
	if (unlikely(err != E_OK)) {
		return _OsRaiseError(err, &record, OSServiceId_IncrementCounter);
	}

	return err;
}

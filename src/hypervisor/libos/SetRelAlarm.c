/*
 * SetRelAlarm.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-08-07: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType SetRelAlarm(AlarmType AlarmID, TickType Increment, TickType Cycle)
{
	OSErrorRecordType record;
	StatusType err;

	record.arg1 = AlarmID;
	record.arg2 = Increment;
	record.arg3 = Cycle;

	err = sys_alarm_set_rel(AlarmID, Increment, Cycle);
	if (unlikely(err != E_OK)) {
		return _OsRaiseError(err, &record, OSServiceId_SetRelAlarm);
	}

	return err;
}

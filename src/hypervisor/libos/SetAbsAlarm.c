/*
 * SetAbsAlarm.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-08-07: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType SetAbsAlarm(AlarmType AlarmID, TickType Start, TickType Cycle)
{
	OSErrorRecordType record;
	StatusType err;

	record.arg1 = AlarmID;
	record.arg2 = Start;
	record.arg3 = Cycle;

	err = sys_alarm_set_abs(AlarmID, Start, Cycle);
	if (unlikely(err != E_OK)) {
		return _OsRaiseError(err, &record, OSServiceId_SetAbsAlarm);
	}

	return err;
}

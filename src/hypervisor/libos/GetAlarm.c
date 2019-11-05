/*
 * GetAlarm.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-08-07: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType GetAlarm(AlarmType AlarmID, TickRefType TickRef)
{
	OSErrorRecordType record;
	StatusType err;

	record.arg1 = AlarmID;
	record.arg2 = (unsigned long)TickRef;

	err = sys_alarm_get(AlarmID, (ctrtick_t*)TickRef);
	if (unlikely(err != E_OK)) {
		return _OsRaiseError(err, &record, OSServiceId_GetAlarm);
	}

	return err;
}

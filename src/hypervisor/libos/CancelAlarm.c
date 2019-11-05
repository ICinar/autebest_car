/*
 * CancelAlarm.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-08-07: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType CancelAlarm(AlarmType AlarmID)
{
	OSErrorRecordType record;
	StatusType err;

	record.arg1 = AlarmID;

	err = sys_alarm_cancel(AlarmID);
	if (unlikely(err != E_OK)) {
		return _OsRaiseError(err, &record, OSServiceId_CancelAlarm);
	}

	return err;
}

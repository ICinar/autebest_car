/*
 * GetAlarmBase.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-08-07: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType GetAlarmBase(AlarmType AlarmID, AlarmBaseRefType InfoRef)
{
	OSErrorRecordType record;
	ctrtick_t maxallowedvalue;
	ctrtick_t ticksperbase;
	ctrtick_t mincycle;
	StatusType err;

	record.arg1 = AlarmID;
	record.arg2 = (unsigned long)InfoRef;

	err = sys_alarm_base(AlarmID, &maxallowedvalue, &ticksperbase, &mincycle);
	if (unlikely(err != E_OK)) {
		return _OsRaiseError(err, &record, OSServiceId_GetAlarmBase);
	}

	InfoRef->maxallowedvalue = maxallowedvalue;
	InfoRef->ticksperbase = ticksperbase;
	InfoRef->mincycle = mincycle;

	return err;
}

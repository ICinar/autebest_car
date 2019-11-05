/*
 * StartScheduleTableAbs.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-08-07: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType StartScheduleTableAbs(ScheduleTableType ScheduleTableID, TickType Start)
{
	OSErrorRecordType record;
	StatusType err;

	record.arg1 = ScheduleTableID;
	record.arg2 = Start;

	err = sys_schedtab_start_abs(ScheduleTableID, Start);
	if (unlikely(err != E_OK)) {
		return _OsRaiseError(err, &record, OSServiceId_StartScheduleTableAbs);
	}

	return err;
}

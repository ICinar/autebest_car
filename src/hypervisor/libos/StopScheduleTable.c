/*
 * StopScheduleTable.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-08-07: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType StopScheduleTable(ScheduleTableType ScheduleTableID)
{
	OSErrorRecordType record;
	StatusType err;

	record.arg1 = ScheduleTableID;

	err = sys_schedtab_stop(ScheduleTableID);
	if (unlikely(err != E_OK)) {
		return _OsRaiseError(err, &record, OSServiceId_StopScheduleTable);
	}

	return err;
}

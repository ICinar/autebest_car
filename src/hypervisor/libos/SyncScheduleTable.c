/*
 * SyncScheduleTable.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-08-07: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType SyncScheduleTable(ScheduleTableType ScheduleTableID, TickType Value)
{
	OSErrorRecordType record;
	StatusType err;

	record.arg1 = ScheduleTableID;
	record.arg2 = Value;

	err = sys_schedtab_sync(ScheduleTableID, Value);
	if (unlikely(err != E_OK)) {
		return _OsRaiseError(err, &record, OSServiceId_SyncScheduleTable);
	}

	return err;
}

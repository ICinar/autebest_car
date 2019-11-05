/*
 * NextScheduleTable.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-08-07: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType NextScheduleTable(ScheduleTableType ScheduleTableID_From, ScheduleTableType ScheduleTableID_To)
{
	OSErrorRecordType record;
	StatusType err;

	record.arg1 = ScheduleTableID_From;
	record.arg2 = ScheduleTableID_To;

	err = sys_schedtab_next(ScheduleTableID_From, ScheduleTableID_To);
	if (unlikely(err != E_OK)) {
		return _OsRaiseError(err, &record, OSServiceId_NextScheduleTable);
	}

	return err;
}

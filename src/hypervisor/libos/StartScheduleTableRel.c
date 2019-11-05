/*
 * StartScheduleTableRel.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-08-07: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType StartScheduleTableRel(ScheduleTableType ScheduleTableID, TickType Offset)
{
	OSErrorRecordType record;
	StatusType err;

	record.arg1 = ScheduleTableID;
	record.arg2 = Offset;

	err = sys_schedtab_start_rel(ScheduleTableID, Offset);
	if (unlikely(err != E_OK)) {
		return _OsRaiseError(err, &record, OSServiceId_StartScheduleTableRel);
	}

	return err;
}

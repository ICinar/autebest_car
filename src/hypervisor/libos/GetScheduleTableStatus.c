/*
 * GetScheduleTableStatus.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-08-07: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

StatusType GetScheduleTableStatus(ScheduleTableType ScheduleTableID, ScheduleTableStatusRefType ScheduleStatusRef)
{
	OSErrorRecordType record;
	unsigned int status;
	StatusType err;

	record.arg1 = ScheduleTableID;
	record.arg2 = (unsigned long)ScheduleStatusRef;

	err = sys_schedtab_get_state(ScheduleTableID, &status);
	if (unlikely(err != E_OK)) {
		err = _OsRaiseError(err, &record, OSServiceId_GetScheduleTableStatus);
	}

	switch (status) {
	case SCHEDTAB_STATE_NEXT:
		*ScheduleStatusRef = SCHEDULETABLE_NEXT;
		break;

	case SCHEDTAB_STATE_WAITING:
		*ScheduleStatusRef = SCHEDULETABLE_WAITING;
		break;

	case SCHEDTAB_STATE_RUNNING:
		*ScheduleStatusRef = SCHEDULETABLE_RUNNING;
		break;

	case SCHEDTAB_STATE_RUNNING_SYNC:
		*ScheduleStatusRef = SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS;
		break;

	case SCHEDTAB_STATE_RUNNING_ASYNC:	/* currently trying to sync */
		*ScheduleStatusRef = SCHEDULETABLE_RUNNING;
		break;

	default:
		/* NOTE: all other states are mapped to STOPPED */
		*ScheduleStatusRef = SCHEDULETABLE_STOPPED;
		break;
	}

	return err;
}

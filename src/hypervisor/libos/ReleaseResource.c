/*
 * ReleaseResource.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-05-04: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"
/* resource configuration */
#include <Os_PrivateCfg.h>

StatusType ReleaseResource(ResourceType ResID)
{
	OSErrorRecordType record;
	TaskType currentTask;
	Os_ResourceData_t* resData;
	StatusType err;
	uint8 prio;

	record.arg1 = ResID;

	if (unlikely(!_OsCallerIsTaskOrIsr())) {
		/* no hooks allowed here! */
		/* FIXME: error not specified by OSEK or AUTOSAR */
		err = E_OS_CALLEVEL;
		goto error;
	}

	if (unlikely(ResID >= Os_numResources)) {
		err = E_OS_ID;
		goto error;
	}

	currentTask = __sys_sched_state.taskid;

	/* no extra critical section protection: we only write resData contents
	 * when we've locked the resource - in this case no one else will
	 * access it
	 */
	resData = &Os_ResourceData[ResID];
	if (resData->lockedBy != currentTask) {
		err = E_OS_NOFUNC;
		goto error;
	}

	/* handle nesting */
	if (Os_lastResTaken[currentTask] != ResID) {
		err = E_OS_NOFUNC;
		goto error;
	}

	Os_lastResTaken[currentTask] = resData->prevResource;
	/* safe a local copy of oldPrio: priority is changed last */
	prio = resData->oldPrio;
	/* release the resource. this must be the last access of resData
	 * contents; we assume the write is atomic
	 */
	barrier();
	resData->lockedBy = INVALID_TASK;
	barrier();

	/* restore prio */
	__sys_sched_state.user_prio = prio;
	barrier();
	if (__sys_sched_state.user_prio < __sys_sched_state.next_prio) {
		sys_fast_prio_sync();
	}

	return E_OK;

	/* error handling */
error:
	return _OsRaiseError(err, &record, OSServiceId_ReleaseResource);
}

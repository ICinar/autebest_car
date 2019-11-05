/*
 * GetResource.c
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

StatusType GetResource(ResourceType ResID)
{
	OSErrorRecordType record;
	TaskType currentTask;
	Os_ResourceData_t* resData;
	StatusType err;
	uint8 prio;

	record.arg1 = ResID;

	if (unlikely(!_OsCallerIsTaskOrIsr())) {
		/* no hooks allowed here */
		/* FIXME: error not specified by OSEK or AUTOSAR */
		err = E_OS_CALLEVEL;
		goto error;
	}

	if (unlikely(ResID >= Os_numResources)) {
		err = E_OS_ID;
		goto error;
	}

	currentTask = __sys_sched_state.taskid;

	/* FIXME missing: [check] the statically assigned priority [...], E_OS_ACCESS  */

	/* FIXME possible additional check: is the current task allowed to access
	   the resource? */

	resData = &Os_ResourceData[ResID];

	/* enter critical section before using resData contents */
	prio = __sys_sched_state.user_prio;
	__sys_sched_state.user_prio = 255;

	if (unlikely(resData->lockedBy != INVALID_TASK)) {
		/* resource already occupied by another task or ISR */
		/* restore prio */
		__sys_sched_state.user_prio = prio;
		/* NOTE: no need to check next_prio here for preemption,
		 * _OsRaiseError() contains another critical section
		 */
		err = E_OS_ACCESS;
		goto error;
	}

	resData->lockedBy = currentTask;
	/* save nesting information and old priority */
	resData->prevResource = Os_lastResTaken[currentTask];
	Os_lastResTaken[currentTask] = ResID;
	resData->oldPrio = prio;

	/* set caller's priority */
	if (prio < resData->prio) {
		prio = resData->prio;
	}

	/* change prio */
	__sys_sched_state.user_prio = prio;
	barrier();
	if (__sys_sched_state.user_prio < __sys_sched_state.next_prio) {
		sys_fast_prio_sync();
	}

	return E_OK;

	/* error handling */
error:
	return _OsRaiseError(err, &record, OSServiceId_GetResource);
}

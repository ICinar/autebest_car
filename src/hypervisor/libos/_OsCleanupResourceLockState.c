/*
 * _OsCleanupResourceLockState.c
 *
 * clean up after a task has finished: release resources locks
 *
 * tjordan, 2014-09-19: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"
/* resource configuration */
#include <Os_PrivateCfg.h>

/** Unlock all resources a task locked (used only in error handling) */
void _OsCleanupResourceLockState(unsigned int task_id)
{
	ResourceType resId;
	Os_ResourceData_t* resData;

	/* clean up resources */
	/* again: no checks/locks: we're still holding all resources that we're
	 * going to access, so we won't get interrupted by someone else who wants
	 * them. */
	resId = Os_lastResTaken[task_id];
	Os_lastResTaken[task_id] = INVALID_RESOURCE;

	while (resId != INVALID_RESOURCE) {
		resData = &Os_ResourceData[resId];
		resData->lockedBy = INVALID_TASK;
		resId = resData->prevResource;
	}
}

/*
 * _OsExceptionHook.c
 *
 * OSEK OS exception handling.
 *
 * azuepke, 2015-03-12: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include <stddef.h>
#include "os_private.h"

void _OsExceptionHook(void *arg __unused)
{
	user_exception_state_t *ex;
	OSErrorRecordType *old_record;
	OSErrorRecordType record;
	ProtectionReturnType ret;

	ex = &_os_exception_state;

	/* setup a fake error record to make GetTaskID() work */
	record.task_id = ex->task_id;
	record.service_id = OSServiceId_INVALID;
	record.error_code = ex->error_code;

	old_record = __OsErrorRecord;
	__OsErrorRecord = &record;
	ret = ProtectionHook(ex->error_code);
	__OsErrorRecord = old_record;

	if (ret == PRO_TERMINATETASKISR) {
		/* enable interrupts if necessary */
		if (_OsSomeoneDisabledInterrupts()) {
			_OsCleanupInterruptLockState();
		}

		/* unlock resources if necessary */
		if (Os_lastResTaken[ex->task_id] != INVALID_RESOURCE) {
			_OsCleanupResourceLockState(ex->task_id);
		}

		sys_task_terminate_other(ex->task_id);
	} else if (ret == PRO_TERMINATEAPPL_RESTART) {
		sys_part_set_operating_mode(PART_OPERATING_MODE_WARM_START);
	} else {
		sys_part_set_operating_mode(PART_OPERATING_MODE_IDLE);
	}

	sys_task_terminate();
	unreachable();
}

/*
 * _OsAsyncErrors.c
 *
 * OSEK OS asynchronous errors (both ErrorHook and ProtectionHook related)
 *
 * azuepke, 2015-03-12: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include <stddef.h>
#include "os_private.h"

/** read position in error array: kernel resets position at partition start */
static uint8_t error_pos = 0;

void _OsAsyncErrorHook(void *arg __unused)
{
	user_error_state_t *ex;
	OSErrorRecordType record;
	ProtectionReturnType ret;
	int prot;

	/* get next error */
	ex = &_os_error_state[error_pos];
	error_pos++;
	if (error_pos >= _os_num_errors) {
		error_pos = 0;
	}

	/* setup a fake error record to make GetTaskID() work */
	record.task_id = ex->task_id;
	record.service_id = OSServiceId_INVALID;
	record.error_code = ex->error_code;

	switch (ex->error_code) {
	case E_OS_LIMIT:	/* ErrorHook: multiple activation */
		record.task_id = (short)INVALID_TASK;
		record.service_id = OSServiceId_ActivateTask;
		record.error_code = ex->error_code;
		record.arg1 = ex->task_id;
		prot = 0;
		break;
	case E_OS_STATE:	/* ErrorHook: set event while SUSPENDED */
		record.task_id = (short)INVALID_TASK;
		record.service_id = OSServiceId_SetEvent;
		record.error_code = ex->error_code;
		record.arg1 = ex->task_id;
		record.arg2 = 1u << ex->extra;	/* bitmask */
		prot = 0;
		break;
	default:			/* ProtectionHook */
		record.task_id = ex->task_id;
		record.service_id = OSServiceId_INVALID;
		record.error_code = ex->error_code;
		prot = 1;
		break;
	}

	ret = PRO_IGNORE;
	if (__OsErrorRecord == NULL) {
		__OsErrorRecord = &record;

		if (prot) {
			ret = ProtectionHook(ex->error_code);
		} else {
			ErrorHook(ex->error_code);
		}

		__OsErrorRecord = NULL;
	}


	if (ret == PRO_IGNORE) {
		/* error ignored */
	} else if (ret == PRO_TERMINATETASKISR) {
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

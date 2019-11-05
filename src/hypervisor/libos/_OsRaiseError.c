/*
 * _OsRaiseError.c
 *
 * OSEK OS error handling.
 *
 * azuepke, 2015-02-27: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include <stddef.h>
#include "os_private.h"

/** indicator if the ErrorHook is currently active */
OSErrorRecordType *__OsErrorRecord = NULL;

StatusType _OsRaiseError(
	StatusType error_code,
	OSErrorRecordType *record,
	OSServiceIdType service_id)
{
	uint8 old_prio;

	/* complete error record */
	record->error_code = error_code;
	record->service_id = service_id;
	record->task_id = __sys_sched_state.taskid;

	/* raise prio */
	old_prio = __sys_sched_state.user_prio;
	__sys_sched_state.user_prio = 255;

	/* prevent recursive invocation of the error hook */
	if (__OsErrorRecord == NULL) {
		__OsErrorRecord = record;

		ErrorHook(record->error_code);

		/* FIXME: SWS_Os_00246: call the application specific error hook next */
		/* FIXME: SWS_Os_00085: call app error hook with application rights */

		__OsErrorRecord = NULL;
	}

	/* restore prio */
	__sys_sched_state.user_prio = old_prio;
	barrier();
	if (__sys_sched_state.user_prio < __sys_sched_state.next_prio) {
		sys_fast_prio_sync();
	}

	return record->error_code;
}

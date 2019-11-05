/*
 * EnableAllInterrupts.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-05-04: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

void EnableAllInterrupts(void)
{
	/* NOTE: this is not suited for ISR cat 2, these run in kernel mode */

	/* restore prio */
	__sys_sched_state.user_prio = __OsPrioBeforeDisableAllInterrupts;
	barrier();
	if (__sys_sched_state.user_prio < __sys_sched_state.next_prio) {
		sys_fast_prio_sync();
	}
}

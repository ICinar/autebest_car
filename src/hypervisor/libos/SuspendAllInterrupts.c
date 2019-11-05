/*
 * SuspendAllInterrupts.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-05-04: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

uint8 __OsPrioBeforeSuspendAllInterrupts = 0;
uint8 __OsSuspendAllInterruptsNesting = 0;

void SuspendAllInterrupts(void)
{
	/* NOTE: this is not suited for ISR cat 2, these run in kernel mode */

	if (__OsSuspendAllInterruptsNesting == 0) {
		/* raise prio */
		__OsPrioBeforeSuspendAllInterrupts = __sys_sched_state.user_prio;
		__sys_sched_state.user_prio = 255;
	}
	__OsSuspendAllInterruptsNesting++;
}

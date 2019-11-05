/*
 * SuspendOSInterrupts.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-05-04: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

uint8 __OsPrioBeforeSuspendOSInterrupts = 0;
uint8 __OsSuspendOSInterruptsNesting = 0;

void SuspendOSInterrupts(void)
{
	/* NOTE: this is not suited for ISR cat 2, these run in kernel mode */

	if (__OsSuspendOSInterruptsNesting == 0) {
		/* raise prio */
		__OsPrioBeforeSuspendOSInterrupts = __sys_sched_state.user_prio;
		__sys_sched_state.user_prio = 255;
	}
	__OsSuspendOSInterruptsNesting++;
}

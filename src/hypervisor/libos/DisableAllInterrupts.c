/*
 * DisableAllInterrupts.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-05-04: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

uint8 __OsPrioBeforeDisableAllInterrupts = 0;

void DisableAllInterrupts(void)
{
	/* NOTE: this is not suited for ISR cat 2, these run in kernel mode */

	/* raise prio */
	__OsPrioBeforeDisableAllInterrupts = __sys_sched_state.user_prio;
	__sys_sched_state.user_prio = 255;
}

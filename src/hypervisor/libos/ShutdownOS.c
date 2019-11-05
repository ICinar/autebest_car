/*
 * ShutdownOS.c
 *
 * OSEK OS API calls.
 *
 * azuepke, 2014-08-07: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"

void ShutdownOS(StatusType error)
{
	/* raise prio so we don't get preempted */
	__sys_sched_state.user_prio = 255;

	ShutdownHook(error);

	/* shutdown partition */
	sys_part_set_operating_mode(PART_OPERATING_MODE_IDLE);
}

/*
 * sys_fast_task_self.c
 *
 * Syscall library fast task ID.
 *
 * azuepke, 2014-05-01: initial
 */

#include <hv.h>
#include "sys_private.h"

unsigned int sys_fast_task_self(void)
{
	return __sys_sched_state.taskid;
}

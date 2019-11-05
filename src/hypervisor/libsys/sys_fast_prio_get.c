/*
 * sys_fast_prio_get.c
 *
 * Syscall library fast priority switching.
 *
 * azuepke, 2014-05-01: initial
 */

#include <hv.h>
#include "sys_private.h"

unsigned int sys_fast_prio_get(void)
{
	return __sys_sched_state.user_prio;
}

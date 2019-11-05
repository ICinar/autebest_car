/*
 * sys_fast_prio_set.c
 *
 * Syscall library fast priority switching.
 *
 * azuepke, 2014-05-01: initial
 */

#include <hv.h>
#include <hv_compiler.h>
#include "sys_private.h"

/* GCC generates better code with a local branch in the function below
 * when not-inlining unnecessary calls to helper functions.
 * passing "old_prio" as variable helps generating better tail-calls.
 */
static __noinline unsigned int sys_fast_prio_set_sync(unsigned int old_prio)
{
	sys_fast_prio_sync();

	return old_prio;
}

/* We can change "user_prio" at will: The kernel synchronizes this
 * value with its internal priority representation at the time a scheduling
 * decision happens (e.g. a thread is placed on the ready queue).
 * The kernel bounds this value between [task.base_prio, part.max_prio].
 *
 * Violating these bounds is our fault and does not harm partition scheduling.
 */
unsigned int sys_fast_prio_set(unsigned int new_prio)
{
	unsigned int old_prio;

	old_prio = __sys_sched_state.user_prio;
	__sys_sched_state.user_prio = new_prio;

	if (__sys_sched_state.user_prio < __sys_sched_state.next_prio) {
		return sys_fast_prio_set_sync(old_prio);
	}

	return old_prio;
}

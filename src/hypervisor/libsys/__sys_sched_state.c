/*
 * __sys_sched_state.c
 *
 * Syscall library private data.
 *
 * azuepke, 2014-05-01: initial
 */

#include "sys_private.h"
#include "hv_compiler.h"

/** user scheduling state */
user_sched_state_t __sys_sched_state __section(.sbss.kernel_shared);

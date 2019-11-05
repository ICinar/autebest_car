/*
 * sys_private.h
 *
 * Syscall library private header.
 *
 * azuepke, 2014-05-01: initial
 */

#ifndef __SYS_PRIVATE_H__
#define __SYS_PRIVATE_H__

#include <hv_types.h>

/** user scheduling state */
extern user_sched_state_t __sys_sched_state;

#endif

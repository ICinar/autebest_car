/*
 * event.h
 *
 * Kernel event handling.
 *
 * azuepke, 2014-03-07: initial
 */

#ifndef __EVENT_H__
#define __EVENT_H__

#include <hv_types.h>

/** Set events in a target task's pending event mask */
__tc_fastcall void sys_ev_set(unsigned int task_id, evmask_t mask);

/** Get events from a target task's pending event mask */
__tc_fastcall void sys_ev_get(unsigned int task_id);

/** Clear a set of events from the caller's task's pending event mask */
__tc_fastcall void sys_ev_clear(evmask_t mask);

/** The caller warits for a set of events to be set in its pending event mask */
__tc_fastcall void sys_ev_wait_get_clear(evmask_t mask, evmask_t clear);

/** Set an event in a task's pending event mask in own or remote partition */
__tc_fastcall void sys_ipev_set(unsigned int ipev_id);


/* internal event set routine */
unsigned int ev_set(struct task *task, evmask_t mask);

#endif

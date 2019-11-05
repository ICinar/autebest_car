/*
 * wq.h
 *
 * ARINC compatible abtract wait queues.
 *
 * azuepke, 2014-09-05: initial
 */

#ifndef __WQ_H__
#define __WQ_H__

#include <wq_state.h>
#include <hv_types.h>

/** static configuration -> config.c */
extern const struct wq_cfg wq_cfg[];

/** initialize all wait queues */
void wq_init_all(void);

/** close a wait queue -- called on partition shutdown/reboot */
void wq_close(struct wq *wq);

/** wake up to "count" tasks on wait queue */
void wq_wake(struct wq *wq, unsigned int count);

/** Set wait queue discipline */
__tc_fastcall void sys_wq_set_discipline(
	unsigned int wq_id,
	unsigned int discipline,
	uint32_t *user_state);

/** Wait on a wait queue */
/* NOTE: we cast prio to a pointer type to pass it in registers on TriCore */
__tc_fastcall void __sys_wq_wait(
	unsigned int wq_id,
	uint32_t compare,
	timeout_t timeout,
	void *prio_casted_as_ptr);

/** Sleep until timeout expires */
__tc_fastcall void sys_sleep(timeout_t timeout);

/** Notify a wait queue */
__tc_fastcall void sys_wq_wake(
	unsigned int wq_id,
	unsigned int count);

/** Cancel sleeping / wake up a sleeping task in the caller's partition */
__tc_fastcall void sys_unblock(unsigned int task_id);

#endif

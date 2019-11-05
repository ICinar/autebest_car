/*
 * task.h
 *
 * Kernel task handling.
 *
 * azuepke, 2013-11-24: initial MPU version
 */

#ifndef __TASK_H__
#define __TASK_H__

#include <task_state.h>
#include <hv_types.h>
#include <assert.h>

/** task configuration -> config.c */
extern const uint16_t num_tasks;

/** initialize idle tasks */
void task_init_idle(void);

/** initialize remaining tasks */
void task_init_rest(void);

/** get a task configuration by its global ID */
static inline const struct task_cfg *task_get_task_cfg(unsigned int task_id)
{
	extern const struct task_cfg task_cfg[];

	assert(task_id < num_tasks);
	return &task_cfg[task_id];
}

/** get the global ID of a task */
static inline unsigned int task_get_global_id(struct task *task)
{
	extern const struct task_cfg task_cfg[];
	unsigned int global_id;

	assert(task != NULL);

	/* FIXME: ugly pointer arithmethic! */
	global_id = task->cfg - task_cfg;
	assert(global_id < num_tasks);

	return global_id;
}


/** check if a task activation will succeed or fail */
unsigned int task_check_activate(struct task *task);
/** activate task or hook */
void task_do_activate(struct task *task);
/** internal preparation for activation of a task */
void task_prepare(struct task *task);
/** terminate a task */
void task_terminate(struct task *task, int partition_shutdown);
/** terminate self */
void task_terminate_self(struct task *task);

/** Retrieve the current task's task ID, including ISRs */
__tc_fastcall void sys_task_self(void);
/** Retrieve the current task's task ID, excluding ISRs */
__tc_fastcall void sys_task_current(void);
/** Retrieve the current ISR's task ID, only for ISRs */
__tc_fastcall void sys_task_isrid(void);
/** Create a task in the caller's partition */
/* NOTE: unlike the user API, the in-kernel function uses ulongs instead of void* */
__tc_fastcall void sys_task_create(
	unsigned int task_id,
	unsigned int prio,
	void *entry,
	void *stack,
	void *arg0,
	void *arg1);
/** Terminate the current task (always return E_OK) */
__tc_fastcall void sys_task_terminate(void);
/** Terminate any other task (except the current one) */
__tc_fastcall void sys_task_terminate_other(unsigned int task_id);
/** Terminate the current task and activate another task */
__tc_fastcall void sys_task_chain(unsigned int task_id);
/** Activate a task */
__tc_fastcall void sys_task_activate(unsigned int task_id);
/** Activate a task and let it wait on a timeout */
__tc_fastcall void sys_task_delayed_activate(unsigned int task_id, int sync, timeout_t timeout);
/** Set priority of a task */
__tc_fastcall void sys_task_set_prio(unsigned int task_id, unsigned int prio);
/** Get priority of a task */
__tc_fastcall void sys_task_get_prio(unsigned int task_id);
/** Get scheduling state of a task */
__tc_fastcall void sys_task_get_state(unsigned int task_id);
/** Mask interrupt source of an interrupt service routine. */
__tc_fastcall void sys_isr_mask(unsigned int isr_id);
/** Unmask interrupt source of an interrupt service routine. */
__tc_fastcall void sys_isr_unmask(unsigned int isr_id);

#endif

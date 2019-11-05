/*
 * sched.h
 *
 * Scheduling related defines.
 *
 * azuepke, 2013-05-04: initial
 * azuepke, 2013-11-24: initial MPU version
 */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <sched_state.h>
#include <hv_types.h>
#include <hv_compiler.h>

/* forward */
struct part;
struct tpschedule_cfg;

/** scheduler configuration -> config.c */
/** Number of processors */
extern const uint8_t num_cpus;
/** Number of time partitions */
extern const uint8_t num_timeparts;

/** initialize scheduler */
void sched_init(void);
/** start scheduling (called once at system start on each CPU) */
void sched_start(void);

/** insert a task at the tail of the ready queue */
void sched_readyq_insert_tail(struct task *task);
/** insert a task at the head of the ready queue */
void sched_readyq_insert_head(struct task *task);
/** remove a task from the ready queue and put task into SUSPENDED state */
void sched_readyq_remove(struct task *task);
/** suspend scheduling for the current task */
void sched_suspend(struct task *task);
/** let current task wait with timeout */
void sched_wait(struct task *task, unsigned int new_state, timeout_t timeout);

/** start the deadline of a task relative to now */
void sched_deadline_start(time_t now, struct task *task);
/** change the deadline of a task to given expiry time */
void sched_deadline_change(time_t deadline, struct task *task);
/** disable the deadline of a task  */
void sched_deadline_disable(struct task *task);

/** the scheduler (called from assembler code) */
__tc_fastcall struct arch_reg_frame *sched_schedule(void);


/** alias to get the current CPU state */
#define current_sched_state()		arch_get_sched_state()

/** alias to get the current task */
#define current_task()				(arch_get_sched_state()->current_task)

/** alias to get the current partition configuration */
#define current_part_cfg()			(arch_get_sched_state()->current_part_cfg)

__tc_fastcall void sys_yield(void);
__tc_fastcall void sys_schedule(void);
__tc_fastcall void sys_fast_prio_sync(void);
unsigned int current_prio_get(void);
void current_prio_set(unsigned int new_prio);

/** Replenish deadline */
__tc_fastcall void sys_replenish(timeout_t budget);
/** Get current system time */
__tc_fastcall void sys_gettime(void);
/** Wait until next partition activation / release point. */
__tc_fastcall void sys_wait_periodic(void);

/** Change time partition schedule on target CPU */
__tc_fastcall void sys_schedule_change(unsigned int cpu_id, unsigned int schedule_id);
void schedule_change(const struct tpschedule_cfg *next_tpschedule);

/** register a pending partition state change */
void sched_enqueue_part_state_change(struct part *part);

#endif

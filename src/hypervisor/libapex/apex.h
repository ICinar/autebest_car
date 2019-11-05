/*
 * apex.h
 *
 * APEX internal lib header
 *
 * azuepke, 2014-09-08: initial
 * azuepke, 2014-09-26: internal locking
 */

#ifndef APEX_H
#define APEX_H

#include <ARINC653.h>
#include <hv_types.h>
#include <hv.h>
#include <assert.h>
#include <hv_error.h>
#include "config.h"

/* use 0xffff as invalid ID */
#define INVALID_ID 0xffff

/** internal state of the APEX library */
extern uint8_t __apex_operating_mode;
extern uint8_t __apex_start_condition;
extern uint8_t __apex_lock_level;
extern uint8_t __apex_error_handler_created;
extern uint16_t __apex_previous_process;

/** user scheduling state */
extern user_sched_state_t __sys_sched_state;

/** APEX initialization */
void __apex_init(void);

/** invocation stub of APEX error handler */
void __apex_error_handler_invoke(void (*func)(void));

/** Application main entry point */
int main(void);

/** translate a kernel error code into an APEX error code */
unsigned int __apex_error_translate(unsigned int err);

/** ARINC configuration mismatch */
void __apex_panic(const char *msg) __noreturn;

/** Compare first 30 characters of APEX names,
 * returnes != 1 if the names are considered equal
 */
int __apex_name_eq(const char *s1, const char *s2);

/** Copy up to 30 characters of APEX names, pad with NUL-characters */
/* NOTE: same as: !strncmp(s1, s2, MAX_NAME_LENGTH) */
void __apex_name_cpy(char *d, const char *s);

/** Find ID of APEX process by name, returns INVALID_ID if not found */
unsigned int __apex_proc_find(const char *name);

/** Find ID of APEX semaphore by name, returns INVALID_ID if not found */
unsigned int __apex_sem_find(const char *name);

/** test if current process is the init hook */
static inline int __apex_is_init_hook(void)
{
	return __sys_sched_state.taskid == __apex_init_hook_id;
}

/** test if current process is the error handler hook */
static inline int __apex_is_error_handler(void)
{
	return __sys_sched_state.taskid == __apex_error_handler_hook_id;
}

/** test if current process refers to itself */
static inline int __apex_is_self(unsigned int id)
{
	return __sys_sched_state.taskid == id;
}

/** test if the partition is in normal mode */
static inline int __apex_in_normal_mode(void)
{
	return __apex_operating_mode == NORMAL;
}

/** test if preemption is disabled */
static inline int __apex_preemption_disabled(void)
{
	return __apex_lock_level != 0;
}

/** lock APEX internally by raising the current process's priority to MCP */
/* NOTE: raise prio regardless of the lock level, checking is more expensive */
static inline void __apex_lock(void)
{
	__sys_sched_state.user_prio = 255;
}

/** unlock APEX again */
/* NOTE: raise prio regardless of the lock level, checking is more expensive */
static inline void __apex_unlock(void)
{
	if (__apex_lock_level == 0) {
		__sys_sched_state.user_prio = __apex_proc_prio[__sys_sched_state.taskid];
		if (unlikely(__sys_sched_state.user_prio < __sys_sched_state.next_prio)) {
			sys_fast_prio_sync();
		}
	}
}

/** unlock APEX again */
/* NOTE: raise prio regardless of the lock level, checking is more expensive */
static inline void __apex_unlock_yield(void)
{
	unsigned int prio;
	uint16_t id;

	assert(__apex_lock_level == 0);

	id = __sys_sched_state.taskid;
	prio = __apex_proc_prio[id];
	sys_task_set_prio(id, prio);
}

#endif

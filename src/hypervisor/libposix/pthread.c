#include "config.h"
#include <hv.h>
#include <hv_error.h>
#include <stdio.h>
#include <stddef.h>

#include "pthread.h"
#include "debug.h"
#include "errno.h"

#include "crit.h"
#include "task.h"
#include "wq.h"

/* user entry point & initial stack */

extern int main(void);

// initial thread entry
static void *initial_pthread(void *arg)
{
	(void)arg;

	main();
	return NULL;
}

/* exported functions */

// Called from __posix_startup to set up internal stuff
void pthread_init(void)
{

	pthread_t thread;
	pthread_attr_t attr;
	int err;

#ifndef NDEBUG
	unsigned int initial_task_id;	// startup hook id
	unsigned int num_wqs;
	unsigned int num_tasks;

	for (unsigned int i = 0; i < __pthread_config_static.num_tasks; i++) {

//		assert(__pthread_config_dynamic.bTaskFree[i] == TASK_FREE);
		assert(__pthread_config_dynamic_btaskfree[i] == TASK_FREE);
//		assert(__pthread_config_dynamic.pStartRoutines[i] == NULL);
		assert(__pthread_config_dynamic_pstartroutines[i] == NULL);
	}
	for (unsigned int i = 0; i < __pthread_config_static.num_waitqueues;
	     i++) {

//		assert(__pthread_config_dynamic.bWaitqueueFree[i] == WQ_FREE);
		assert(__pthread_config_dynamic_bwaitqueuefree[i] == WQ_FREE);
	}

	// count the number of wait queues in this partition
	for (num_wqs = 0;; num_wqs++) {
		if (E_OS_LIMIT == sys_wq_wait(num_wqs, 42, 0, 0))
			break;
	}
	assert(num_wqs == __pthread_config_static.num_waitqueues);

	// count number of tasks in this partition
	for (num_tasks = 0;; num_tasks++) {
		int task_state;
		if (E_OK != sys_task_get_state(num_tasks, &task_state))
			break;
		if (task_state != TASK_STATE_SUSPENDED)
			break;
	}
	assert(num_tasks == __pthread_config_static.num_tasks);

	initial_task_id = sys_fast_task_self();
	DEBUG_PRINTF
	    ("pthread_init: ID of the initial task (partition start hook) is %d\n",
	     initial_task_id);

	assert(MAX_TASKS - 1 <= 0xFFFF);
	assert(MAX_WQS - 1 <= 0xFFFF);

#endif

	DEBUG_PRINTF
	    ("pthread_init: %d suspended tasks in this partition (this is our thread pool)\n",
	     __pthread_config_static.num_tasks);
	DEBUG_PRINTF
	    ("pthread_init: %d wait queues in this partition (this is our mutex/cond/joinables pool)\n",
	     __pthread_config_static.num_waitqueues);

	assert(sys_fast_prio_get() < __CRIT_PRIO_MAX);	// Below the ceiling priority?
	//FIXME: assert(sys_fast_task_self() == ???);	// Called from startup hook?

	DEBUG_PRINTF("pthread_init: Starting first actual task ...\n");
	err = pthread_attr_init(&attr);
	assert(err == EOK);
	err =
	    pthread_attr_setstack(&attr,
//				  &__stack_main[PTHREAD_STACKSIZE - 16],
				  &__stack_main[__pthread_config_static.initial_thread_stack_size - 16],
				  __pthread_config_static.initial_thread_stack_size);
	assert(err == EOK);
	err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	assert(err == EOK);
	err = pthread_create(&thread, &attr, initial_pthread, NULL);
	assert(err == EOK);
}

/* pthread API functions */

/* internal task entry */
static void _task_entry(void *arg)
{

	void *ret;
	int task_id = sys_fast_task_self();
	assert(sys_fast_prio_get() < __CRIT_PRIO_MAX);
	DEBUG_PRINTF("Thread %d starts up\n", task_id);

	// call entry point
//	ret = __pthread_config_dynamic.pStartRoutines[task_id] (arg);
	ret = __pthread_config_dynamic_pstartroutines[task_id] (arg);

	DEBUG_PRINTF
	    ("Thread %d returned with return code 0x%p ... terminating\n",
//	     sys_fast_task_self(), __pthread_config_dynamic.pRetVal[task_id]);
	     sys_fast_task_self(), __pthread_config_dynamic_pretval[task_id]);
	pthread_exit(ret);
}

int
pthread_create(pthread_t * thread, const pthread_attr_t * attr,
	       void *(*start_routine) (void *), void *arg)
{

	int task_id;
	int err;
	assert(attr != NULL);
	assert(thread != NULL);
	assert(start_routine != NULL);

	// check attributes
	if ((attr->stackaddr == 0) || (attr->stacksize == 0) ||
	    ((attr->detachstate != PTHREAD_CREATE_DETACHED)
	     && (attr->detachstate != PTHREAD_CREATE_JOINABLE))
	    ||
	    ((attr->sched_param.sched_priority < PTHREAD_PRIO_MIN
	      || attr->sched_param.sched_priority > PTHREAD_PRIO_MAX))) {
		return EINVAL;	// EINVAL: The attributes specified by attr are invalid.
	}

	task_id = alloc_free_task();
	if (task_id == -1) {
		DEBUG_PRINTF
		    ("pthread_create(): Could not create thread; No free task available\n");
		return EAGAIN;	// EAGAIN: The system lacked the necessary resources to create another thread.
	}

//	__pthread_config_dynamic.pStartRoutines[task_id] = start_routine;
	__pthread_config_dynamic_pstartroutines[task_id] = start_routine;
	if (attr->detachstate == PTHREAD_CREATE_JOINABLE) {
		int wq = alloc_free_wq();
		if (wq == -1) {
			DEBUG_PRINTF
			    ("pthread_create(): Could not create joinable thread; No free wait queue available\n");
			return EAGAIN;	// EAGAIN: The system lacked the necessary resources to create another thread.
		}
		err = sys_wq_set_discipline(wq - 1, WQ_DISCIPLINE_PRIO);
		assert(err == E_OK);

		//__pthread_config_dynamic.bJoinable[task_id] = 1;
//		__pthread_config_dynamic.joiner_queues[task_id] = wq;
		__pthread_config_dynamic_joinerqueues[task_id] = wq;
	} else {
		assert(attr->detachstate == PTHREAD_CREATE_DETACHED);
//		__pthread_config_dynamic.joiner_queues[task_id] = 0;
		__pthread_config_dynamic_joinerqueues[task_id] = 0;
	}

	*thread = task_id;

	err =
	    sys_task_create(task_id, attr->sched_param.sched_priority,
			    _task_entry, attr->stackaddr, arg, NULL);
	DEBUG_PRINTF("pthread_create(): sys_task_create() returned 0x%x\n",
		     err);
	assert(err == E_OK);	// FIXME: handle possible syscall errors
	DEBUG_PRINTF("pthread_create(): Created task %d with stack 0x%p\n",
		     task_id, attr->stackaddr);

	return EOK;
}

pthread_t pthread_self(void) {
	pthread_t ret = sys_fast_task_self();
	return ret;
}

int pthread_once(pthread_once_t* once_control,  void (*init_routine)(void)) {

	assert(once_control != NULL);
	assert(init_routine != NULL);

	if (*once_control == PTHREAD_ONCE_INIT) {
		init_routine();
		*once_control = ~PTHREAD_ONCE_INIT;
	}
	return 0;
}

int pthread_join(pthread_t thread, void **retval)
{
	int wq;
	int err;
	unsigned int crit;

	if (thread >= __pthread_config_static.num_tasks)
		return ESRCH;	// ESRCH: No thread could be found corresponding to that specified by the given thread ID.

//	if (__pthread_config_dynamic.bTaskFree[thread] != TASK_INUSE)
	if (__pthread_config_dynamic_btaskfree[thread] != TASK_INUSE)
		return ESRCH;	// ESRCH: No thread could be found corresponding to that specified by the given thread ID.

//	if (__pthread_config_dynamic.joiner_queues[thread] == 0)
	if (__pthread_config_dynamic_joinerqueues[thread] == 0)
		return EINVAL;	// EINVAL: The implementation has detected that the value specified by thread does not refer to a joinable thread.

	// enqueue ourselves at the joinee
//	wq = __pthread_config_dynamic.joiner_queues[thread];
	wq = __pthread_config_dynamic_joinerqueues[thread];
	assert(wq != 0);

	DEBUG_PRINTF("pthread_join(): waiting for joinee to exit\n");

	crit = __crit_enter();
	err = sys_wq_wait(wq - 1, 0, 1, 0);
	assert(err == EOK);
	DEBUG_PRINTF("pthread_join(): joinee exited: Return to caller\n");

	if (retval != NULL)
//		*retval = __pthread_config_dynamic.pRetVal[thread];
		*retval = __pthread_config_dynamic_pretval[thread];
	__crit_leave(crit);

	return EOK;
}

void pthread_exit(void *value_ptr)
{
	unsigned int crit;
	unsigned int task_id = sys_fast_task_self();

//	__pthread_config_dynamic.pRetVal[task_id] = value_ptr;
	__pthread_config_dynamic_pretval[task_id] = value_ptr;
	DEBUG_PRINTF("pthread_exit() for thread %d, return code = 0x%p\n",
//		     task_id, __pthread_config_dynamic.pRetVal[task_id]);
		     task_id, __pthread_config_dynamic_pretval[task_id]);

	crit = __crit_enter();

	// free this task
	free_task(task_id);

//	if (__pthread_config_dynamic.joiner_queues[task_id] != 0) {
	if (__pthread_config_dynamic_joinerqueues[task_id] != 0) {
		// wake all joiners
		int err;
//		int wq = __pthread_config_dynamic.joiner_queues[task_id];
		int wq = __pthread_config_dynamic_joinerqueues[task_id];
		err = sys_wq_wake(wq - 1, (unsigned int)-1);	// wake all waiters
		assert(err == EOK);
		free_wq(wq);
//		__pthread_config_dynamic.joiner_queues[task_id] = 0;
		__pthread_config_dynamic_joinerqueues[task_id] = 0;
	}
	// thread returned: terminate it
	sys_task_terminate();
	// the following lines are never executed
	// the critical section is left by means of sys_task_terminate()
	// Another pthread_create() to reuse this task will reinitialize task priority anyway
	assert(0);
	__crit_leave(crit);
}

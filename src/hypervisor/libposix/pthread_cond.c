#include <hv.h>
#include <hv_error.h>
#include <stdio.h>
#include <stddef.h>

#include "pthread.h"
#include "errno.h"
#include "cond.h"
#include "crit.h"

#include "wq.h"
#include "mutex.h"

#define __TIMESPEC_VALID(pts) (pts->tv_nsec <= 999999999)

int pthread_cond_init(pthread_cond_t * cond, const pthread_condattr_t * attr)
{
	assert(cond != NULL);
	assert(attr != NULL);

	if (cond->wq_id != 0)
		return EBUSY;	// EBUSY: The implementation has detected an attempt to reinitialize the object referenced by cond, a previously initialized, but not yet destroyed, condition variable.

	return __init_cond(cond);
}

int pthread_cond_destroy(pthread_cond_t * cond)
{
	unsigned int crit;
	assert(cond != NULL);

	if (!__COND_INITIALIZED(cond)) {
		return EINVAL;	// EINVAL: The value cond does not refer to an initialized condition variable.
	}

	crit = __crit_enter();

	if (cond->waiters != 0) {
		__crit_leave(crit);
		return EBUSY;	// EBUSY: The implementation has detected an attempt to destroy the object referenced by cond while it is referenced
	}
	// free waitqueue for subsequent allocation
	free_wq(cond->wq_id);

	cond->wq_id = 0;
	__crit_leave(crit);

	return EOK;
}

int pthread_cond_signal(pthread_cond_t * cond)
{
	assert(cond != NULL);

	if (!__COND_INITIALIZED(cond))
		return EINVAL;	// EINVAL: The value cond does not refer to an initialized condition variable.

	// unblock the highest priority thread waiting on the condition variable
	__cond_signal(cond);

	return EOK;
}

int pthread_cond_broadcast(pthread_cond_t * cond)
{
	assert(cond != NULL);

	if (!__COND_INITIALIZED(cond))
		return EINVAL;	// EINVAL: The value cond does not refer to an initialized condition variable.

	// unblock all threads waiting on the condition variable
	__cond_broadcast(cond);

	return EOK;
}

int pthread_cond_wait(pthread_cond_t * cond, pthread_mutex_t * mutex)
{
	unsigned int crit;
	int err;
	int prio = sys_fast_prio_get();
	assert(prio <= PTHREAD_PRIO_MAX);
	assert(cond != NULL);
	assert(mutex != NULL);

	if (!__COND_INITIALIZED(cond))
		return EINVAL;	// EINVAL: The value cond does not refer to an initialized condition variable.

	if (!__MUTEX_INITIALIZED(mutex))
		return EINVAL;	// EINVAL: The value cond does not refer to an initialized condition variable.

	if (mutex->owner != sys_fast_task_self())
		return EINVAL;	// EINVAL: The mutex was not owned by the current thread at the time of the call.

	crit = __crit_enter();
	// atomically release mutex and block on condition variable
	crit = __do_unlock_mutex(mutex, mutex->protocol, crit);

	// Wait for signal
	__do_cond_wait(cond, prio);

	__crit_leave(crit);

	// lock mutex
	err = pthread_mutex_lock(mutex);
	assert(err == EOK);

	return EOK;
}

int
pthread_cond_timedwait_rel(pthread_cond_t * restrict cond,
			   pthread_mutex_t * restrict mutex,
			   const struct timespec *restrict rel_timeout)
{

	unsigned int crit;
	int err;
	int ret;
	timeout_t timeout;
	int prio = sys_fast_prio_get();
	assert(prio <= PTHREAD_PRIO_MAX);
	assert(cond != NULL);
	assert(mutex != NULL);

	if (!__COND_INITIALIZED(cond))
		return EINVAL;	// EINVAL: The value cond does not refer to an initialized condition variable.

	if (!__MUTEX_INITIALIZED(mutex))
		return EINVAL;	// EINVAL: The value cond does not refer to an initialized condition variable.

	if (!__TIMESPEC_VALID(rel_timeout))
		return EINVAL;	// EINVAL: The value cond does not refer to an initialized condition variable.

	if (mutex->owner != sys_fast_task_self())
		return EINVAL;	// EINVAL: The mutex was not owned by the current thread at the time of the call.

	timeout = rel_timeout->tv_sec * 1000000000 + rel_timeout->tv_nsec;

	crit = __crit_enter();
	// atomically release mutex and block on condition variable
	crit = __do_unlock_mutex(mutex, mutex->protocol, crit);
	ret = __do_cond_timedwait(cond, timeout, prio);
	__crit_leave(crit);

	// lock mutex
	err = pthread_mutex_lock(mutex);
	assert(err == EOK);

	return ret;
}

#include <hv.h>
#include <hv_error.h>
#include <stdio.h>
#include <stddef.h>

#include "pthread.h"
#include "config.h"
#include "errno.h"

#include "mutex.h"

#include "crit.h"
#include "wq.h"

#define __TIMESPEC_VALID(pts) (pts->tv_nsec <= 999999999)

int
pthread_mutex_init(struct pthread_mutex_str *mutex,
		   const pthread_mutexattr_t * mutexattr)
{

	int type;
	int protocol;

	assert(mutex != NULL);
	assert(mutexattr != NULL);

	type = mutexattr->type;
	protocol = mutexattr->protocol;

	if ((type != PTHREAD_MUTEX_NORMAL)
	    && (type != PTHREAD_MUTEX_ERRORCHECK)
	    && (type != PTHREAD_MUTEX_RECURSIVE))
		return EINVAL;	// EINVAL: The value specified by attr is invalid.

	if ((protocol != PTHREAD_PRIO_NONE)
	    && (protocol != PTHREAD_PRIO_INHERIT)
	    && (protocol != PTHREAD_PRIO_PROTECT))
		return EINVAL;	// EINVAL: The value specified by attr is invalid.

	if (mutex->state != __MUTEX_STATE_UNINITIALIZED)
		return EBUSY;	// EBUSY: The implementation has detected an attempt to re-initialise the object referenced by mutex, a previously initialised, but not yet destroyed, mutex.

	return __init_mutex(mutex, type, protocol);
}

int pthread_mutex_lock(struct pthread_mutex_str *mutex)
{
	assert(mutex != NULL);

	if (!__MUTEX_INITIALIZED(mutex))
		return EINVAL;	// EINVAL: The value specified by mutex does not refer to an initialised mutex object.

	return __lock_mutex(mutex, mutex->protocol);
}

int pthread_mutex_trylock(struct pthread_mutex_str *mutex)
{
	assert(mutex != NULL);

	if (!__MUTEX_INITIALIZED(mutex))
		return EINVAL;	// EINVAL: The value specified by mutex does not refer to an initialised mutex object.

	return __trylock__mutex(mutex);
}

int
pthread_mutex_timedlock_rel(pthread_mutex_t * restrict mutex,
			    const struct timespec *restrict rel_timeout)
{

	timeout_t timeout;
	assert(mutex != NULL);
	assert(rel_timeout != NULL);

	if (!__MUTEX_INITIALIZED(mutex))
		return EINVAL;	// EINVAL: The value specified by mutex does not refer to an initialised mutex object.

	if (!__TIMESPEC_VALID(rel_timeout))
		return EINVAL;	// EINVAL: The value cond does not refer to an initialized condition variable.

	timeout = rel_timeout->tv_sec * 1000000000 + rel_timeout->tv_nsec;
	return __lock_mutex_timeout(mutex, timeout);
}

int pthread_mutex_unlock(struct pthread_mutex_str *mutex)
{
	assert(mutex != NULL);

	if (!__MUTEX_INITIALIZED(mutex))
		return EINVAL;	// EINVAL: The value specified by mutex does not refer to an initialised mutex object.

	if ((mutex->type == PTHREAD_MUTEX_ERRORCHECK)
	    || (mutex->type == PTHREAD_MUTEX_RECURSIVE)) {
		if ((mutex->state == __MUTEX_STATE_UNLOCKED)
		    || (mutex->owner != sys_fast_task_self()))
			return EPERM;	// EPERM: The current thread does not own the mutex.
	}

	return __unlock_mutex(mutex, mutex->protocol);
}

int pthread_mutex_destroy(struct pthread_mutex_str *mutex)
{
	unsigned int crit;
	assert(mutex != NULL);

	if (!__MUTEX_INITIALIZED(mutex)) {
		return EINVAL;	// EINVAL: The value specified by mutex is invalid.
	}

	crit = __crit_enter();

	// check state
	if (mutex->state != __MUTEX_STATE_UNLOCKED) {
		__crit_leave(crit);
		return EBUSY;	// EBUSY: The implementation has detected an attempt to destroy the object referenced by mutex while it is locked or referenced
		// FIXME: Also return EBUSY if mutex is being used in a pthread_cond_wait() or pthread_cond_timedwait()) by another thread.
	}

	assert(mutex->wq_id < __pthread_config_static.num_waitqueues);
	// free waitqueue for subsequent allocation
	free_wq(mutex->wq_id);

	// unitialize
	mutex->wq_id = 0;
	mutex->state = __MUTEX_STATE_UNINITIALIZED;
	mutex->type = 0;
	mutex->waiters = 0;
	mutex->owner = 0;

	__crit_leave(crit);

	return EOK;
}

int
pthread_mutex_setprioceiling(pthread_mutex_t * mutex,
			     int prioceiling, int *old_ceiling)
{

	int err;
	assert(mutex != NULL);
	assert(old_ceiling != NULL);

	if (!__MUTEX_INITIALIZED(mutex)) {
		return EINVAL;	// EINVAL: The value specified by mutex does not refer to a currently existing mutex.
	}

	if ((prioceiling <= PTHREAD_PRIO_MIN)
	    || (prioceiling > PTHREAD_PRIO_MAX)) {
		return EINVAL;	// EINVAL: The priority requested by prioceiling is out of range.
	}
	// lock the mutex, set its ceiling priority and unlock it again
	err = __lock_mutex(mutex, PTHREAD_PRIO_NONE);
	if (err == EOK) {

		// set ceiling prio, return old ceiling
		*old_ceiling = mutex->ceiling_prio;
		mutex->ceiling_prio = prioceiling;
		err = __unlock_mutex(mutex, PTHREAD_PRIO_NONE);
		assert(err == EOK);
	}
	return err;
}

int
pthread_mutex_getprioceiling(const pthread_mutex_t * mutex, int *prioceiling)
{
	assert(mutex != NULL);
	assert(prioceiling != NULL);

	if (!__MUTEX_INITIALIZED(mutex)) {
		return EINVAL;	// EINVAL: The value specified by mutex does not refer to a currently existing mutex.
	}

	return mutex->ceiling_prio;
}

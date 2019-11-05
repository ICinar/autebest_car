#include <stddef.h>
#include <hv.h>
#include <hv_error.h>
#include <stdio.h>

#include "pthread.h"
#include "config.h"
#include "debug.h"
#include "errno.h"
#include "mutex.h"
#include "wq.h"
#include "crit.h"

#define TO_WAITER_GET_PRIO(x) ((x) >> 24)
#define TO_WAITER_GET_TASKID(x) ((x) & 0xFFFF)
#define NEW_TO_WAITER(prio, taskid) (((prio) << 24) | (taskid & 0xFFFF))

/*
	Inherit the given priority to the owner of the given mutex if it is higehr than its current priority.
	If the given priority is higher than the "boosted_prio_by_non_to_waiters" attribute of the mutex, set it accordingly.
*/
static void inherit_prio_to_owner(struct pthread_mutex_str *m,
				  unsigned int prio)
{

	unsigned int ownerprio;
	int err;
	err = sys_task_get_prio(m->owner, &ownerprio);
	assert(err == E_OK);
	if (prio > m->boosted_prio_by_non_to_waiters)
		m->boosted_prio_by_non_to_waiters = prio;

	if (prio > ownerprio) {
		DEBUG_PRINTF
		    ("Inheriting prio %d to mutex owner %d (old prio = %d)\n",
		     prio, m->owner, ownerprio);
		err = sys_task_set_prio(m->owner, prio);
		assert(err == E_OK);
	}
}

/*
	Inherit the given priority to the owner of the given mutex if it is higehr than its current priority.
	Do NOT set the "boosted_prio_by_non_to_waiters" attribute of the mutex.
*/
static void inherit_timeout_prio_to_owner(struct pthread_mutex_str *m,
					  unsigned int prio)
{

	unsigned int ownerprio;
	int err;
	err = sys_task_get_prio(m->owner, &ownerprio);
	assert(err == E_OK);
	if (prio > ownerprio) {
		DEBUG_PRINTF
		    ("Inheriting prio %d to mutex owner %d (old prio = %d)\n",
		     prio, m->owner, ownerprio);
		err = sys_task_set_prio(m->owner, prio);
		assert(err == E_OK);
	}
}

/*
	After a mutex timeout for a thread has expired and it has been removed from the timeout queue:
	  Eventually lower the priority of the mutex owner to
	  reflect that the timed-out thread no longer inherits its priority to the mutex owner.
*/
static void uninherit_timeout_prio_to_owner(struct pthread_mutex_str *m)
{
	int err;
	unsigned int i;
	unsigned int new_owner_prio = m->boosted_prio_by_non_to_waiters;

	// calculate new boosted prio for mutex owner
	for (i = 0; i < m->num_to_waiters; i++) {
		if (TO_WAITER_GET_PRIO(m->to_waiters[i]) > new_owner_prio)
			new_owner_prio = TO_WAITER_GET_PRIO(m->to_waiters[i]);
	}

	// set new prio of owner
	DEBUG_PRINTF
	    ("Undo prio inheritance by timed-out waiter: (New) owner prio = %d\n",
	     new_owner_prio);
	err = sys_task_set_prio(m->owner, new_owner_prio);
	assert(err == E_OK);
}

/*
	Enqueue the given taskid "waiter" with the given priority "prio" into the timeout queue of specified mutex "m".
*/
static void enqueue_timeout_waiter(struct pthread_mutex_str *m,
				   unsigned int waiter, unsigned int prio)
{
	// enqueue ourselves as a "timeout waiter"
	m->to_waiters[m->num_to_waiters++] = NEW_TO_WAITER(prio, waiter);
}

/*
	Dequeue the given taskid "waiter" from the timeout queue of specified mutex "m".
*/
static void dequeue_timeout_waiter(struct pthread_mutex_str *m,
				   unsigned int waiter)
{
	unsigned int i;
	for (i = 0; i < m->num_to_waiters; i++) {
		if (TO_WAITER_GET_TASKID(m->to_waiters[i]) == waiter) {
			m->to_waiters[i] = m->to_waiters[m->num_to_waiters];
			m->num_to_waiters--;
			return;
		}
	}
	assert(0);
}

int __init_mutex(struct pthread_mutex_str *m, int type, int protocol)
{

	int wqid;
	int err;
	assert(m != NULL);

	wqid = alloc_free_wq();
	if (wqid == -1)
		return EAGAIN;	// EGAIN: The system lacked the necessary resources (other than memory) to initialise another mutex.

	m->wq_id = wqid;
	m->state = __MUTEX_STATE_UNLOCKED;
	m->waiters = 0;
	m->type = type;
	m->protocol = protocol;
	m->ceiling_prio = PTHREAD_PRIO_MAX;
	m->owner = 0;
	m->num_to_waiters = 0;

	err = sys_wq_set_discipline(m->wq_id - 1, WQ_DISCIPLINE_PRIO);
	assert(err == E_OK);
	return EOK;
}

/*
	sys_wq_wait() at the waitqueue of the given mutex "m" with the given priority "prio" and timeout "to"
*/
static int wait_for_mutex(struct pthread_mutex_str *m, unsigned int prio,
			  timeout_t to)
{
	int err;
	m->state = __MUTEX_STATE_HASWAITERS;
	m->waiters++;
	DEBUG_PRINTF("Thread %d waits for mutex\n", sys_fast_task_self());
	err =
	    sys_wq_wait(m->wq_id - 1, __MUTEX_STATE_HASWAITERS, to, prio);
	m->waiters--;

	if (m->waiters == 0)
		m->state = __MUTEX_STATE_LOCKED;

	if (err == E_OK) {
		DEBUG_PRINTF("Thread %d woke up to aquire mutex\n",
			     sys_fast_task_self());
	} else {
		DEBUG_PRINTF("Thread %d timed out waiting or mutex\n",
			     sys_fast_task_self());
	}
	return err;
}

int __lock_mutex(struct pthread_mutex_str *m, int protocol)
{

	unsigned int crit;
	int err;
	unsigned int task_id = sys_fast_task_self();

	assert(m != NULL);

	crit = __crit_enter();
	if ((protocol == PTHREAD_PRIO_PROTECT) && (crit > m->ceiling_prio)) {
		__crit_leave(crit);
		return EINVAL;	// EINVAL: The mutex was created with the protocol attribute having the value PTHREAD_PRIO_PROTECT and the calling thread's priority is higher than the mutex's current priority ceiling.
	}

	if (m->state == __MUTEX_STATE_UNLOCKED) {
		// lock is free
		m->state = __MUTEX_STATE_LOCKED;
	} else {
		if ((m->type == PTHREAD_MUTEX_ERRORCHECK)
		    && (m->owner == task_id)) {
			__crit_leave(crit);
			return EDEADLK;
		}
		if ((m->type == PTHREAD_MUTEX_RECURSIVE)
		    && (m->owner == task_id)) {
			m->lock_count++;
			__crit_leave(crit);
			return EOK;
		}

		if (protocol == PTHREAD_PRIO_INHERIT) {
			// if we have higher priority -> boost the priority of the mutex owner
			inherit_prio_to_owner(m, crit);
		}
		// lock is not free: enqueue ourselves
		err = wait_for_mutex(m, crit, -1);
		assert(err == E_OK);
	}

	assert(m->lock_count == 0);
	m->lock_count = 1;
	assert(m->owner == 0);
	m->owner = task_id;
	m->owner_original_prio = crit;
	m->boosted_prio_by_non_to_waiters = 0;

	if (protocol == PTHREAD_PRIO_PROTECT) {
		// boost our current priority to the maximum of our current priority and the priority ceiling of the mutex
		unsigned int myprio = crit;
		unsigned int ceiling = m->ceiling_prio;

		if (m->ceiling_prio > myprio) {
			DEBUG_PRINTF
			    ("Setting prio of mutex owner from %d to mutex ceiling priority %d\n",
			     myprio, ceiling);
			crit = ceiling;
		}
	}
#ifndef NDEBUG
	// if we aquired the mutex, then there is per definition no higher priority waiter with a timeout
	{
		unsigned int i;
		for (i = 0; i < m->num_to_waiters; i++) {
			assert(TO_WAITER_GET_PRIO(m->to_waiters[i]) <= crit);
		}
	}
#endif

	__crit_leave(crit);

	return EOK;
}

int __trylock__mutex(struct pthread_mutex_str *m)
{
	unsigned int crit;
	int ret;
	assert(m != NULL);
	crit = __crit_enter();
	if ((m->protocol == PTHREAD_PRIO_PROTECT) && (crit > m->ceiling_prio)) {
		__crit_leave(crit);
		return EINVAL;	// EINVAL: The mutex was created with the protocol attribute having the value PTHREAD_PRIO_PROTECT and the calling thread's priority is higher than the mutex's current priority ceiling.
	}

	if (m->state == __MUTEX_STATE_UNLOCKED) {
		// lock is free
		m->state = __MUTEX_STATE_LOCKED;
		m->lock_count = 1;
		assert(m->owner == 0);
		m->owner = sys_fast_task_self();
		m->owner_original_prio = sys_fast_prio_get();
		m->boosted_prio_by_non_to_waiters = 0;

		if (m->protocol == PTHREAD_PRIO_PROTECT) {
			// boost our current priority to the maximum of our current priority and the priority ceiling of the mutex
			unsigned int myprio = crit;
			unsigned int ceiling = m->ceiling_prio;

			if (m->ceiling_prio > myprio) {
				DEBUG_PRINTF
				    ("Setting prio of mutex owner from %d to mutex ceiling priority %d\n",
				     myprio, ceiling);
				crit = ceiling;
			}
		}

		ret = EOK;

#ifndef NDEBUG
		// if we aquired the mutex, then there is per definition no higher priority waiter with a timeout
		{
			unsigned int i;
			for (i = 0; i < m->num_to_waiters; i++) {
				assert(TO_WAITER_GET_PRIO(m->to_waiters[i]) <=
				       crit);
			}
		}
#endif

	} else {
		ret = EBUSY;
	}
	__crit_leave(crit);
	return ret;
}

int __lock_mutex_timeout(struct pthread_mutex_str *m, timeout_t timeout)
{

	unsigned int crit;
	int err;
	unsigned int task_id;

	assert(m != NULL);
	task_id = sys_fast_task_self();
	crit = __crit_enter();

	if ((m->protocol == PTHREAD_PRIO_PROTECT) && (crit > m->ceiling_prio)) {
		__crit_leave(crit);
		return EINVAL;	// EINVAL: The mutex was created with the protocol attribute having the value PTHREAD_PRIO_PROTECT and the calling thread's priority is higher than the mutex's current priority ceiling.
	}

	if (m->state == __MUTEX_STATE_UNLOCKED) {
		// lock is free
		m->state = __MUTEX_STATE_LOCKED;
	} else {
		if (timeout == 0) {
			__crit_leave(crit);
			return ETIMEDOUT;
		}
		if ((m->type == PTHREAD_MUTEX_ERRORCHECK)
		    && (m->owner == task_id)) {
			__crit_leave(crit);
			return EDEADLK;
		}
		if ((m->type == PTHREAD_MUTEX_RECURSIVE)
		    && (m->owner == task_id)) {
			m->lock_count++;
			__crit_leave(crit);
			return EOK;
		}

		if (m->protocol == PTHREAD_PRIO_INHERIT) {

			if (m->num_to_waiters ==
				__pthread_config_static.max_timeouts_per_prio_inherit_mutex) {
				__crit_leave(crit);
				return ESYS;	// TODO: Document this! Maximum number of timeout-waiters on a PRIO_INHERIT mutex.
			}

			inherit_timeout_prio_to_owner(m, crit);
			enqueue_timeout_waiter(m, task_id, crit);
		}

		err = wait_for_mutex(m, crit, timeout);
		if (err == E_OS_TIMEOUT) {

			if (m->protocol == PTHREAD_PRIO_INHERIT) {

				// deqeuue ourselves from the timeout queue
				dequeue_timeout_waiter(m, task_id);
				// eventually unboost the owner's priority
				uninherit_timeout_prio_to_owner(m);
			}

			__crit_leave(crit);
			return ETIMEDOUT;
		}
		if (m->protocol == PTHREAD_PRIO_INHERIT) {
			// EOK: Remove us from timeout queue
			dequeue_timeout_waiter(m, task_id);
		}
	}
	assert(m->lock_count == 0);
	m->lock_count = 1;
	assert(m->owner == 0);
	m->owner = task_id;
	m->owner_original_prio = crit;
	m->boosted_prio_by_non_to_waiters = 0;

	if (m->protocol == PTHREAD_PRIO_PROTECT) {
		// boost our current priority to the maximum of our current priority and the priority ceiling of the mutex
		unsigned int myprio = crit;
		unsigned int ceiling = m->ceiling_prio;
		if (m->ceiling_prio > myprio)
			crit = ceiling;
	}
#ifndef NDEBUG
	// if we aquired the mutex, then there is per definition no higher priority waiter with a timeout
	{
		unsigned int i;
		for (i = 0; i < m->num_to_waiters; i++) {
			assert((m->to_waiters[i] >> 24) <= crit);
		}
	}
#endif

	__crit_leave(crit);
	return EOK;
}

unsigned int
__do_unlock_mutex(struct pthread_mutex_str *m, int protocol, unsigned int crit)
{

	unsigned int prio = sys_fast_prio_get();
	assert(prio == __CRIT_PRIO_MAX);

	m->lock_count--;
	assert(m->lock_count >= 0);
	assert((m->type == PTHREAD_MUTEX_RECURSIVE) || (m->lock_count == 0));

	if (m->lock_count == 0) {
		m->owner = 0;
		if (m->state == __MUTEX_STATE_LOCKED) {
			m->state = __MUTEX_STATE_UNLOCKED;
		} else {
			assert(m->state == __MUTEX_STATE_HASWAITERS);
			if (m->waiters > 0)
				sys_wq_wake(m->wq_id - 1, 1);
		}
	}

	if (protocol == PTHREAD_PRIO_INHERIT) {
		// set back our priority
		DEBUG_PRINTF
		    ("Relowering prio of previous mutex owner from inherited prio %d to %d\n",
		     crit, m->owner_original_prio);
		crit = m->owner_original_prio;
	}
	if (protocol == PTHREAD_PRIO_PROTECT) {
		DEBUG_PRINTF
		    ("Relowering prio of previous mutex owner from ceiling prio %d to %d\n",
		     crit, m->owner_original_prio);
		crit = m->owner_original_prio;
	}
	return crit;
}

int __unlock_mutex(struct pthread_mutex_str *m, int protocol)
{
	unsigned int crit;

	crit = __crit_enter();
	crit = __do_unlock_mutex(m, protocol, crit);
	__crit_leave(crit);
	return EOK;
}

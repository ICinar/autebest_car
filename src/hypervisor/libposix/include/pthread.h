#ifndef _PTHREAD_H_
#define _PTHREAD_H_

#include <stdint.h>
#include <time.h>
#include "config.h"

/* Threads: definitions */

#define PTHREAD_PRIO_MAX	254
#define PTHREAD_PRIO_MIN	0

struct sched_param {
	int sched_priority;
};

typedef struct pthread_attr_str {
	struct sched_param sched_param;
	void *stackaddr;
	size_t stacksize;
	int detachstate;
	int sched_policy;
} pthread_attr_t;

typedef unsigned int pthread_t;
typedef unsigned int pthread_once_t;


#define SCHED_FIFO					0x10

#define PTHREAD_CREATE_DETACHED		0x10
#define PTHREAD_CREATE_JOINABLE		0x20

#define PTHREAD_ONCE_INIT			((pthread_once_t)0x9F)

#ifndef MAX_TIMEOUTS_PER_PRIO_INHERIT_MUTEX
#define MAX_TIMEOUTS_PER_PRIO_INHERIT_MUTEX 3
#endif

/* Mutexes: definitions */

typedef struct pthread_mutexattr_str {
	int type;
	int protocol;
} pthread_mutexattr_t;

typedef struct pthread_mutex_str {
	unsigned char protocol;	// PI avoidance protocol
	unsigned char owner_original_prio;	// Original priority of mutex owner
	unsigned char ceiling_prio;	// Ceiling priority of mutex
	unsigned int state;	// state (free, locked, contented)
	unsigned short wq_id;	// UID of the underlying wait queue
	unsigned short waiters;	// number of waiters in the queue
	unsigned short owner;	// task id of the mutex owner
	unsigned char type;	// mutex type (normal, errorcheck, recursive)
	int lock_count;		// for recursive mutexes: the lock count

	unsigned char boosted_prio_by_non_to_waiters;	// For PTHREAD_PRIO_INHERIT-mutexes: The priority value the mutex owner would have if no waiters
	// would be in the queue which are subject to a timeout
	unsigned short num_to_waiters;	// The number of waiters in to_waiters (and correspondingly, in the wait queue) which are subject to a timeout.
	unsigned int to_waiters[MAX_TIMEOUTS_PER_PRIO_INHERIT_MUTEX];	// List of timeout-affected waiters: Highest byte of each value is the priority of the waiter,
	// lowest two bytes is the task id of the waiter.
} pthread_mutex_t;

#define PTHREAD_MUTEX_DEFAULT		PTHREAD_MUTEX_NORMAL
#define PTHREAD_MUTEX_NORMAL		0x10
#define PTHREAD_MUTEX_ERRORCHECK	0x20
#define PTHREAD_MUTEX_RECURSIVE		0x30

/* When a thread owns a mutex with the PTHREAD_PRIO_NONE protocol attribute,
 * its priority and scheduling are not affected by its mutex ownership.
 */
#define	PTHREAD_PRIO_NONE			0x10
/*
 * When a thread is blocking higher priority threads because of owning one
 * or more mutexes with the PTHREAD_PRIO_INHERIT protocol attribute,
 * it executes at the higher of its priority or the priority of the highest
 * priority thread waiting on any of the mutexes owned by this thread and
 * initialised with this protocol.
 */
#define	PTHREAD_PRIO_INHERIT		0x20
/*
 * When a thread owns one or more mutexes initialised with the
 * PTHREAD_PRIO_PROTECT protocol, it executes at the higher of its priority
 * or the highest of the priority ceilings of all the mutexes owned by this
 * thread and initialised with this attribute, regardless of whether other
 * threads are blocked on any of these mutexes or not.
 */
#define	PTHREAD_PRIO_PROTECT		0x30

/* Condition variables: definitions */

typedef struct pthread_condattr_str {
	// empty
} pthread_condattr_t;

typedef struct pthread_cond_str {
	int wq_id;		// UID of the underlying wait queue
	int waiters;		// number of waiters in the queue
} pthread_cond_t;

/* Library initialization */

void pthread_init(void);

/* POSIX functions */

int pthread_mutexattr_init(pthread_mutexattr_t * attr);
int pthread_mutexattr_destroy(pthread_mutexattr_t * attr);

int pthread_mutexattr_settype(pthread_mutexattr_t * attr, int type);
int pthread_mutexattr_gettype(const pthread_mutexattr_t * attr, int *type);

int pthread_mutexattr_setprotocol(pthread_mutexattr_t * attr, int protocol);
int pthread_mutexattr_getprotocol(const pthread_mutexattr_t * attr,
				  int *protocol);

int pthread_mutex_init(pthread_mutex_t * mutex,
		       const pthread_mutexattr_t * mutexattr);
int pthread_mutex_lock(pthread_mutex_t * mutex);
int pthread_mutex_trylock(pthread_mutex_t * mutex);
int pthread_mutex_timedlock_rel(pthread_mutex_t * restrict mutex,
				const struct timespec *restrict rel_timeout);
int pthread_mutex_unlock(pthread_mutex_t * mutex);
int pthread_mutex_destroy(pthread_mutex_t * mutex);

int pthread_mutex_setprioceiling(pthread_mutex_t * mutex, int prioceiling,
				 int *old_ceiling);
int pthread_mutex_getprioceiling(const pthread_mutex_t * mutex,
				 int *prioceiling);

int pthread_create(pthread_t * thread, const pthread_attr_t * attr,
		   void *(*start_routine) (void *), void *arg);

int pthread_attr_init(pthread_attr_t * attr);
int pthread_attr_destroy(pthread_attr_t * attr);
int pthread_attr_setstack(pthread_attr_t * attr, void *stackaddr,
			  size_t stacksize);
int pthread_attr_setstackaddr(pthread_attr_t * attr, void *stackaddr);
int pthread_attr_setstacksize(pthread_attr_t * attr, size_t stacksize);
int pthread_attr_setschedparam(pthread_attr_t * attr,
			       const struct sched_param *param);
int pthread_attr_setdetachstate(pthread_attr_t * attr, int detachstate);
int pthread_attr_setschedpolicy(pthread_attr_t * attr, int policy);
int pthread_attr_getschedpolicy(pthread_attr_t * attr, int *policy);

int pthread_attr_getstack(pthread_attr_t * attr, void **stackaddr,
			  size_t * stacksize);
int pthread_attr_getstackaddr(pthread_attr_t * attr, void **stackaddr);
int pthread_attr_getstacksize(pthread_attr_t * attr, size_t * stacksize);
int pthread_attr_getschedparam(pthread_attr_t * attr,
			       struct sched_param *param);
int pthread_attr_getdetachstate(pthread_attr_t * attr, int *detachstate);

int pthread_join(pthread_t thread, void **retval);
void pthread_exit(void *value_ptr);
pthread_t pthread_self(void);
int pthread_once(pthread_once_t* once_control,  void (*init_routine)(void));

int pthread_condattr_init(pthread_condattr_t * attr);
int pthread_condattr_destroy(pthread_condattr_t * attr);

int pthread_cond_init(pthread_cond_t * cond, const pthread_condattr_t * attr);
int pthread_cond_destroy(pthread_cond_t * cond);
int pthread_cond_wait(pthread_cond_t * cond, pthread_mutex_t * mutex);
int pthread_cond_timedwait_rel(pthread_cond_t * restrict cond,
			       pthread_mutex_t * restrict mutex,
			       const struct timespec *restrict rel_timeout);
int pthread_cond_signal(pthread_cond_t * cond);
int pthread_cond_broadcast(pthread_cond_t * cond);

#endif

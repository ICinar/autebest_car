#include "pthread.h"
#include "config.h"
#include "wq.h"
#include "crit.h"

/* Find a wait queue of the partition which is currently unused and return in its id or -1 if none is found */
int alloc_free_wq(void)
{

	unsigned int id;
	unsigned int crit;
	crit = __crit_enter();
	for (id = 0; id < __pthread_config_static.num_waitqueues; id++) {
//		if (__pthread_config_dynamic.bWaitqueueFree[id] == WQ_FREE) {
		if (__pthread_config_dynamic_bwaitqueuefree[id] == WQ_FREE) {
//			__pthread_config_dynamic.bWaitqueueFree[id] = WQ_INUSE;
			__pthread_config_dynamic_bwaitqueuefree[id] = WQ_INUSE;
			__crit_leave(crit);
			return id + 1;
		}
	}
	__crit_leave(crit);
	return -1;		// none free
}

void free_wq(int id)
{

	id -= 1;
	assert((id >= 0)
	       && ((unsigned int)id < __pthread_config_static.num_waitqueues));
//	assert(__pthread_config_dynamic.bWaitqueueFree[id] == WQ_INUSE);
	assert(__pthread_config_dynamic_bwaitqueuefree[id] == WQ_INUSE);
//	__pthread_config_dynamic.bWaitqueueFree[id] = WQ_FREE;
	__pthread_config_dynamic_bwaitqueuefree[id] = WQ_FREE;
}

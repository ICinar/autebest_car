#include "task.h"
#include "debug.h"
#include "crit.h"
#include "config.h"

/* Find a task of the partition which is currently unused and return in its id or -1 if none is found */
int alloc_free_task(void)
{

	unsigned int id;
	unsigned int crit;
	crit = __crit_enter();
	for (id = 0; id < __pthread_config_static.num_tasks; id++) {
//		if (__pthread_config_dynamic.bTaskFree[id]) {
		if (__pthread_config_dynamic_btaskfree[id]) {
//			__pthread_config_dynamic.bTaskFree[id] = TASK_INUSE;
			__pthread_config_dynamic_btaskfree[id] = TASK_INUSE;
			__crit_leave(crit);
			return id;
		}
	}
	__crit_leave(crit);
	return -1;
}

void free_task(int id)
{

	assert((id >= 0)
	       && ((unsigned int)id < __pthread_config_static.num_tasks));
//	assert(__pthread_config_dynamic.bTaskFree[id] == TASK_INUSE);
	assert(__pthread_config_dynamic_btaskfree[id] == TASK_INUSE);
//	__pthread_config_dynamic.bTaskFree[id] = TASK_FREE;
	__pthread_config_dynamic_btaskfree[id] = TASK_FREE;
}

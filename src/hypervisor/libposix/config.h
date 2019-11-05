#ifndef _CONFIG_H_
#define _CONFIG_H_

#define MAX_TIMEOUTS_PER_PRIO_INHERIT_MUTEX 3

#define TASK_INUSE 0
#define TASK_FREE  1

#define WQ_INUSE 0
#define WQ_FREE  1

struct __pthread_config_static_str {

	const unsigned int num_tasks;
	const unsigned int num_waitqueues;
	const unsigned int initial_thread_stack_size;
	const unsigned int max_timeouts_per_prio_inherit_mutex;
};

extern void *(*__pthread_config_dynamic_pstartroutines[]) (void *);
extern void *__pthread_config_dynamic_pretval[];
extern int __pthread_config_dynamic_joinerqueues[];
extern unsigned char __pthread_config_dynamic_btaskfree[];
extern unsigned char __pthread_config_dynamic_bwaitqueuefree[];

extern struct __pthread_config_static_str __pthread_config_static;
extern char __stack_main[];

#endif

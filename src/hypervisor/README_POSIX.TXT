
*** uwi POSIX-library

** Features:

Thread creation (SCHED_FIFO) & thread self-termination (pthread_exit), pthread_once()
Mutexes (normal, errorcheck, recursive, priority ceiling, priority inheritance)
Condition variables (pthread_cond_wait, pthread_cond_signal, pthread_cond_broadcast)
No dynamic memory allocations
Bounded runtime.

** Limitations/Flavor:

ALl handles are, in structure, transparent to the application and can directly be compared (ok) or altered (bad). The application 
is assumed to behave correctly.

Threads directly map to uwi tasks, thus the number of tasks for each partition limits the number of pthreads that can be started.
This includes the initially started thread which calls main(). Threads can not be killed from the outside, the only means for a thread to exit 
is calling pthread_exit() or returning from its "start_routine".

Mutexes, Condition variables and joinable threads directly map to uwi wait queues. Thus, in sum, you can not have more of these objects
than waitqueues are defined in the partition configuration at the same time.

Timeout-Waiters on a mutex with the priority inheritance protocol requires additional memory to be reserved for each mutex handle. The 
definition NUM_TIMEOUTS_PER_PRIO_PROTECT_MUTEX (config.h) allows to adjust this value. The default is three. pthread_mutex_lock() will fail with 
errorcode ESYS when too much concurrent timeout waiters try to lock a mutex.

Stacks are entirely handled in user code. Before a thread can be created with pthread_create(), stack address and size has to be set 
in the corresponding pthread_attr_t instance. Undefined behavior occurs, when stacks addresses or sizes are invalid or used concurrently. 
If you want to safely reuse a thread's stack, you should join that thread by means of pthread_join() before. The initial pthread that calls the
user main function is passed a statically allocated stack of size PTHREAD_STACKSIZE (config.h)


** On generating the static configuration for the uwi POSIX library

The partition configuration has to be parsed for POSIX partitions:
	- Partitions are identified either explicitly or by a certain naming scheme, TBD
	- Partiitons maximum priority must be the maximum of 255.

In each POSIX partition, there is one entry hook which points to __posix_startup.
In each POSIX partition, there is a number of tasks which should be configured to allow their blocking and timeouts.
	- Dont care about the naming. Just count the number and ensure sufficient privileges to be configured.
In each POSIX partition, there is a number of wait queues.
	- Dont care about the naming. Just count the number and make sure they are all linked to the partition they are contained in.

For each POSIX partition, we need to identify the coresponding application source code directory, in order to create the "config.c" and "config.h" into it.

POSIX configuration data is distinguished into two types: static (ro) data and dynamic (rw) data.
The static data has the following structure (config.h): 

struct __pthread_config_static_str {

	const unsigned int num_tasks;
	const unsigned int num_waitqueues;
	const unsigned int initial_thread_stack_size;
	const unsigned int max_timeouts_per_prio_inherit_mutex;
};

It can be initialized as follows:

#define NUM_TASKS 5
#define NUM_WQS	5

struct __pthread_config_static_str __pthread_config_static = {
	NUM_TASKS,			// num_tasks
	NUM_WQS,			// num_waitqueues
	1024,				// initial thread stacksize
	3					// maximum number of timeout-waiters on PRIO_INHERIT mutex
};

The dynamically used bits have the following structure (config.h):

extern void *(*__pthread_config_dynamic_pstartroutines[]) (void *);
extern void *__pthread_config_dynamic_pretval[];
extern int __pthread_config_dynamic_joinerqueues[];
extern unsigned char __pthread_config_dynamic_btaskfree[];
extern unsigned char __pthread_config_dynamic_bwaitqueuefree[];

and can be initialized by array initializers like the following (config.c). Note that the lengths of the initializer fields 
have to match the corresponding values in the static initializer.

void *(*__pthread_config_dynamic_pstartroutines[NUM_TASKS]) (void *) = 
	{NULL, NULL, NULL, NULL, NULL};	// pStartRoutines
void *__pthread_config_dynamic_pretval[NUM_TASKS] =
	{NULL, NULL, NULL, NULL, NULL};	// pRetVal
int __pthread_config_dynamic_joinerqueues[NUM_TASKS] = 
	{-1, -1, -1, -1, -1};	// joiner_queues
unsigned char __pthread_config_dynamic_btaskfree[NUM_TASKS] =
	{TASK_FREE, TASK_FREE, TASK_FREE, TASK_FREE, TASK_FREE};	// bTaskFree
unsigned char __pthread_config_dynamic_bwaitqueuefree[NUM_WQS] = 
	{WQ_FREE, WQ_FREE, WQ_FREE, WQ_FREE, WQ_FREE};	// bWaitqueueFree


* This leads to the following algorithm to map the partition configuration to a partition's application level POSIX configuration:

1) Parse the parititon configuration for all partitions of type "POSIX". Make sure partition ceiling prio is 255.
2) Find the application source code directory.
3) Count the number of tasks <M>. Make sure all tasks are sufficiently privileged.
4) Count the number of waitqueues <N>. Make sure each waitqueue is linked to the containing partition.
5) Make sure the inithook points to "__posix_startup()".
6) Generate the configuration as described above.
7) Recompile the application (to add config.c)

* Discussion

How to identify a POSIX partition? Preliminary answer: By the follwoing naming scheme:

posix_[a-zA-Z0-9]+

Consider also the next bullet.

* Further thoughts

Now, one can think of additional XML-nodes in a higher-abstracted configuration which then would be translated down to an actual uwi-partition-configuration.
For example, sth. like

<posix_partition num_tasks="M" num_wqs="N">

is sufficient to be translated to a valid POSIX partition configuration. The names would nicely be autogenerated 
according to some straightforward scheme here.

Another option would be to specify a certain name scheme explicitly for POSIX partitions and just provide shortcuts for 
the task and waitqueue definitions:

<partition name="posix_kunterbuntLeckerFroschsalat">
	<posix_tasks num="M"/>
	<posix_waitqueues num="N"/>
</partition>

* Current implementation

Refer to libposix/templates/* and app5/gen_config.sh


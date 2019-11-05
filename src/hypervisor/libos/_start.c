/*
 * _start.c
 *
 * start up an OSEK partition.
 *
 * tjordan, 2014-09-19: initial
 */

#include <Os_Api.h>
#include <hv.h>
#include "os_private.h"
/* stack configuration */
#include <Os_PrivateCfg.h>

#ifndef NDEBUG
#include <stdio.h>
extern const char __libos_buildid[];
#endif

/* this is the code that runs in the init_hook - it calls main(), and that is
 * supposed to call StartOS(). */
void _start(void)
{
#ifndef NDEBUG
	printf("==== startup OSEK partition\npartition %d: hello world!\n", sys_part_self());
	printf("libos buildid: %s\n", __libos_buildid);
#endif

	/* FIXME: something else to do here (e.g. config checks)? */

	/* call main */
	main();
	/* should not come here */
	sys_task_terminate();
	sys_abort();
}

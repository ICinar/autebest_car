/*
 * kldd.c
 *
 * Kernel level device drivers.
 *
 * azuepke, 2014-03-09: initial
 */

#include <kernel.h>
#include <assert.h>
#include <part.h>
#include <task.h>
#include <kldd.h>
#include <sched.h>
#include <hv_error.h>


/** Call a kernel level device driver (KLDD) identified by its ID kldd_id */
void sys_kldd_call(unsigned int kldd_id, unsigned long arg1,
                           unsigned long arg2, unsigned long arg3)
{
	const struct part_cfg *part_cfg;
	const struct kldd_cfg *kldd;
	unsigned int err;

	part_cfg = current_part_cfg();

	if (kldd_id >= part_cfg->num_kldds) {
		SET_RET(E_OS_ID);
		return;
	}
	kldd = &part_cfg->kldds[kldd_id];

	err = kldd->func(kldd->arg0, arg1, arg2, arg3);
	SET_RET(err);
}


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

// NOTE: KLDD test code below -- delete!

unsigned int kldd_argA;
unsigned int kldd_argB;
unsigned int kldd_argC;

/* mandatory prototypes */
unsigned int kldd_funcA(void *arg0, unsigned long arg1,
                        unsigned long arg2, unsigned long arg3);
unsigned int kldd_funcB(void *arg0, unsigned long arg1,
                        unsigned long arg2, unsigned long arg3);
unsigned int kldd_funcC(void *arg0, unsigned long arg1,
                        unsigned long arg2, unsigned long arg3);


unsigned int kldd_funcA(void *arg0, unsigned long arg1,
                        unsigned long arg2, unsigned long arg3)
{
	assert(arg0 == &kldd_argA);
	(void)arg0;
	(void)arg1;
	(void)arg2;
	(void)arg3;

	//printf("kldd_funcA %p %lu %lu %lu\n", arg0, arg1, arg2, arg3);

	return 42;
}

unsigned int kldd_funcB(void *arg0, unsigned long arg1,
                        unsigned long arg2, unsigned long arg3)
{
	assert(arg0 == &kldd_argB);
	(void)arg0;
	(void)arg1;
	(void)arg2;
	(void)arg3;

	printf("kldd_funcB %p %lu %lu %lu\n", arg0, arg1, arg2, arg3);

	return 47;
}

unsigned int kldd_funcC(void *arg0, unsigned long arg1,
                        unsigned long arg2, unsigned long arg3)
{
	assert(arg0 == &kldd_argC);
	(void)arg0;
	(void)arg1;
	(void)arg2;
	(void)arg3;

	printf("kldd_funcC %p %lu %lu %lu\n", arg0, arg1, arg2, arg3);

	return 11;
}

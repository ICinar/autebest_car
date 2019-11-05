#include <stddef.h>
#include <hv.h>
#include <hv_error.h>
#include "debug.h"
#include "errno.h"
#include "wq.h"
#include "cond.h"
#include "crit.h"

int __init_cond(struct pthread_cond_str *cond)
{

	int wqid;
	int err;
	assert(cond != NULL);

	wqid = alloc_free_wq();
	if (wqid == -1)
		return EAGAIN;	// EAGAIN: The system lacked the necessary resources (other than memory) to initialise another condition variable.

	cond->waiters = 0;
	err = sys_wq_set_discipline(wqid - 1, WQ_DISCIPLINE_PRIO);
	assert(err == E_OK);
	cond->wq_id = wqid;
	return EOK;
}

void __do_cond_wait(struct pthread_cond_str *cond, int prio)
{
	int err;
	cond->waiters++;
	err = sys_wq_wait(cond->wq_id - 1, 42, 42 /* FIXME */, -1, prio);
	assert(err == E_OK);
	cond->waiters--;
}

void __cond_wait(struct pthread_cond_str *cond, int prio)
{

	unsigned int crit;
	assert(prio <= PTHREAD_PRIO_MAX);

	crit = __crit_enter();
	__do_cond_wait(cond, prio);
	__crit_leave(crit);

}

int
__do_cond_timedwait(struct pthread_cond_str *cond, timeout_t timeout, int prio)
{
	int err;
	cond->waiters++;
	err = sys_wq_wait(cond->wq_id - 1, 42 /* FIXME */, timeout, prio);
	cond->waiters--;
	return err;
}

int __cond_timedwait(struct pthread_cond_str *cond, timeout_t timeout, int prio)
{

	int err;
	unsigned int crit;
	assert(prio <= PTHREAD_PRIO_MAX);

	crit = __crit_enter();
	err = __do_cond_timedwait(cond, timeout, prio);
	__crit_leave(crit);

	assert((err == EOK) || (err == E_OS_TIMEOUT));
	return err;
}

void __cond_signal(struct pthread_cond_str *cond)
{

	int err;
	err = sys_wq_wake(cond->wq_id - 1, 1);	// wake highest priority waiter
	assert(err == EOK);
}

void __cond_broadcast(struct pthread_cond_str *cond)
{

	int err;
	err = sys_wq_wake(cond->wq_id - 1, -1);	// wake all waiters
	assert(err == EOK);
}

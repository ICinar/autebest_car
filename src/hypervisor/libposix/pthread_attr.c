#include <hv.h>
#include <hv_error.h>
#include <stdio.h>
#include <stddef.h>
#include "config.h"

#include "pthread.h"
#include "errno.h"

int pthread_attr_init(pthread_attr_t * attr)
{

	assert(attr != NULL);
	attr->stackaddr = NULL;
	attr->stacksize = 0;
	attr->sched_param.sched_priority = sys_fast_prio_get();
	attr->detachstate = PTHREAD_CREATE_DETACHED;
	attr->sched_policy = SCHED_FIFO;
	return EOK;
}

int pthread_attr_destroy(pthread_attr_t * attr)
{
	assert(attr != NULL);

	if ((attr->detachstate != PTHREAD_CREATE_DETACHED) &&
	    (attr->detachstate != PTHREAD_CREATE_JOINABLE))
		return EINVAL;	// EINVAL: The value specified by attr does not refer to an initialized thread attribute object.

	attr->stackaddr = NULL;
	attr->stacksize = 0;
	attr->sched_param.sched_priority = 0;
	attr->detachstate = 0;
	attr->sched_policy = 0;

	return EOK;
}

int
pthread_attr_setstack(pthread_attr_t * attr, void *stackaddr, size_t stacksize)
{

	assert(attr != NULL);

	if ((attr->detachstate != PTHREAD_CREATE_DETACHED) &&
	    (attr->detachstate != PTHREAD_CREATE_JOINABLE))
		return EINVAL;	// EINVAL: The value of the attribute being set is not valid.

	attr->stackaddr = stackaddr;
	attr->stacksize = stacksize;
	return E_OK;
}

int pthread_attr_setstackaddr(pthread_attr_t * attr, void *stackaddr)
{
	assert(attr != NULL);

	if ((attr->detachstate != PTHREAD_CREATE_DETACHED) &&
	    (attr->detachstate != PTHREAD_CREATE_JOINABLE))
		return EINVAL;	// EINVAL: The value of the attribute being set is not valid.

	attr->stackaddr = stackaddr;
	return EOK;
}

int pthread_attr_setstacksize(pthread_attr_t * attr, size_t stacksize)
{
	assert(attr != NULL);

	if ((attr->detachstate != PTHREAD_CREATE_DETACHED) &&
	    (attr->detachstate != PTHREAD_CREATE_JOINABLE))
		return EINVAL;	// EINVAL: The value of the attribute being set is not valid.

	attr->stacksize = stacksize;
	return EOK;
}

int
pthread_attr_setschedparam(pthread_attr_t * attr,
			   const struct sched_param *param)
{
	assert(attr != NULL);
	assert(param != NULL);

	if ((attr->detachstate != PTHREAD_CREATE_DETACHED) &&
	    (attr->detachstate != PTHREAD_CREATE_JOINABLE))
		return EINVAL;	// EINVAL: The value of the attribute being set is not valid.

	if ((param->sched_priority < PTHREAD_PRIO_MIN)
	    || (param->sched_priority > PTHREAD_PRIO_MAX))
		return ENOTSUP;	// ENOTSUP: An attempt was made to set the attribute to an unsupported value.

	attr->sched_param = *param;
	return E_OK;
}

int pthread_attr_setschedpolicy(pthread_attr_t * attr, int policy)
{
	assert(attr != NULL);

	if ((attr->detachstate != PTHREAD_CREATE_DETACHED) &&
	    (attr->detachstate != PTHREAD_CREATE_JOINABLE))
		return EINVAL;	// EINVAL: The value of the attribute being set is not valid.

	if (policy != SCHED_FIFO)
		return EINVAL;	// EINVAL: Invalid value in policy

	attr->sched_policy = policy;
	return EOK;
}

int pthread_attr_getschedpolicy(pthread_attr_t * attr, int *policy)
{
	assert(attr != NULL);
	assert(policy != NULL);

	if ((attr->detachstate != PTHREAD_CREATE_DETACHED) &&
	    (attr->detachstate != PTHREAD_CREATE_JOINABLE))
		return EINVAL;	// EINVAL: The value of the attribute being set is not valid.

	*policy = attr->sched_policy;
	return EOK;
}

int pthread_attr_setdetachstate(pthread_attr_t * attr, int detachstate)
{
	assert(attr != NULL);

	if ((attr->detachstate != PTHREAD_CREATE_DETACHED) &&
	    (attr->detachstate != PTHREAD_CREATE_JOINABLE))
		return EINVAL;	// EINVAL: The value of the attribute being set is not valid.

	if ((detachstate != PTHREAD_CREATE_JOINABLE)
	    && (detachstate != PTHREAD_CREATE_DETACHED))
		return EINVAL;	// EINVAL: The value of detachstate was not valid

	attr->detachstate = detachstate;
	return EOK;
}

int
pthread_attr_getstack(pthread_attr_t * attr,
		      void **stackaddr, size_t * stacksize)
{
	assert(attr != NULL);
	assert(stackaddr != NULL);
	assert(stacksize != NULL);

	if ((attr->detachstate != PTHREAD_CREATE_DETACHED) &&
	    (attr->detachstate != PTHREAD_CREATE_JOINABLE))
		return EINVAL;	// EINVAL: The value of the attribute being set is not valid.

	*stackaddr = attr->stackaddr;
	*stacksize = attr->stacksize;
	return EOK;
}

int pthread_attr_getstackaddr(pthread_attr_t * attr, void **stackaddr)
{
	assert(attr != NULL);
	assert(stackaddr != NULL);

	if ((attr->detachstate != PTHREAD_CREATE_DETACHED) &&
	    (attr->detachstate != PTHREAD_CREATE_JOINABLE))
		return EINVAL;	// EINVAL: The value of the attribute being set is not valid.

	*stackaddr = attr->stackaddr;
	return EOK;
}

int pthread_attr_getstacksize(pthread_attr_t * attr, size_t * stacksize)
{
	assert(attr != NULL);
	assert(stacksize != NULL);

	if ((attr->detachstate != PTHREAD_CREATE_DETACHED) &&
	    (attr->detachstate != PTHREAD_CREATE_JOINABLE))
		return EINVAL;	// EINVAL: The value of the attribute being set is not valid.

	*stacksize = attr->stacksize;
	return EOK;
}

int pthread_attr_getschedparam(pthread_attr_t * attr, struct sched_param *param)
{
	assert(attr != NULL);
	assert(param != NULL);

	if ((attr->detachstate != PTHREAD_CREATE_DETACHED) &&
	    (attr->detachstate != PTHREAD_CREATE_JOINABLE))
		return EINVAL;	// EINVAL: The value of the attribute being set is not valid.

	*param = attr->sched_param;
	return EOK;
}

int pthread_attr_getdetachstate(pthread_attr_t * attr, int *detachstate)
{
	assert(attr != NULL);
	assert(detachstate != NULL);

	if ((attr->detachstate != PTHREAD_CREATE_DETACHED) &&
	    (attr->detachstate != PTHREAD_CREATE_JOINABLE))
		return EINVAL;	// EINVAL: The value of the attribute being set is not valid.

	*detachstate = attr->detachstate;
	return EOK;
}

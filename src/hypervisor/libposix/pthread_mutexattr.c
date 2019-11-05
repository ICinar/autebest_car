#include <hv.h>
#include <hv_error.h>
#include <stdio.h>
#include <stddef.h>

#include "pthread.h"
#include "errno.h"

int pthread_mutexattr_init(pthread_mutexattr_t * attr)
{
	assert(attr != NULL);
	attr->type = PTHREAD_MUTEX_DEFAULT;
	attr->protocol = PTHREAD_PRIO_NONE;

	return EOK;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t * attr)
{
	// nop
	assert(attr != NULL);

	if ((attr->type != PTHREAD_MUTEX_NORMAL)
	    && (attr->type != PTHREAD_MUTEX_ERRORCHECK)
	    && (attr->type != PTHREAD_MUTEX_RECURSIVE))
		return EINVAL;	// EINVAL: The value specified by attr is invalid.

	attr->type = 0;

	return EOK;
}

int pthread_mutexattr_settype(pthread_mutexattr_t * attr, int type)
{
	assert(attr != NULL);

	if ((attr->type != PTHREAD_MUTEX_NORMAL)
	    && (attr->type != PTHREAD_MUTEX_ERRORCHECK)
	    && (attr->type != PTHREAD_MUTEX_RECURSIVE))
		return EINVAL;	// EINVAL: The value specified by attr is invalid.

	if ((type != PTHREAD_MUTEX_NORMAL)
	    && (type != PTHREAD_MUTEX_ERRORCHECK)
	    && (type != PTHREAD_MUTEX_RECURSIVE))
		return EINVAL;	// EINVAL: The value type is invalid.
	attr->type = type;
	return EOK;
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t * attr, int *type)
{
	assert(attr != NULL);

	*type = attr->type;
	if ((*type != PTHREAD_MUTEX_NORMAL)
	    && (*type != PTHREAD_MUTEX_ERRORCHECK)
	    && (*type != PTHREAD_MUTEX_RECURSIVE))
		return EINVAL;	// EINVAL: The value specified by attr is invalid.
	return EOK;
}

int pthread_mutexattr_setprotocol(pthread_mutexattr_t * attr, int protocol)
{
	assert(attr != NULL);

	if ((attr->type != PTHREAD_MUTEX_NORMAL)
	    && (attr->type != PTHREAD_MUTEX_ERRORCHECK)
	    && (attr->type != PTHREAD_MUTEX_RECURSIVE))
		return EINVAL;	// EINVAL: The value specified by attr is invalid.

	if ((protocol != PTHREAD_PRIO_NONE)
	    && (protocol != PTHREAD_PRIO_INHERIT)
	    && (protocol != PTHREAD_PRIO_PROTECT))
		return EINVAL;	// EINVAL: The value specified by protocol is invalid.

	attr->protocol = protocol;
	return EOK;
}

int
pthread_mutexattr_getprotocol(const pthread_mutexattr_t * attr, int *protocol)
{
	assert(attr != NULL);
	assert(protocol != NULL);

	if ((attr->type != PTHREAD_MUTEX_NORMAL)
	    && (attr->type != PTHREAD_MUTEX_ERRORCHECK)
	    && (attr->type != PTHREAD_MUTEX_RECURSIVE))
		return EINVAL;	// EINVAL: The value specified by attr is invalid.

	*protocol = attr->protocol;
	return EOK;
}

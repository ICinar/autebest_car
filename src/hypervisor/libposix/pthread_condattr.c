#include <hv.h>
#include <hv_error.h>
#include <stdio.h>
#include <stddef.h>

#include "pthread.h"
#include "errno.h"

int pthread_condattr_init(pthread_condattr_t * attr)
{
	assert(attr != NULL);
	// nop
	return EOK;
}

int pthread_condattr_destroy(pthread_condattr_t * attr)
{
	assert(attr != NULL);
	// nop
	return EOK;
}

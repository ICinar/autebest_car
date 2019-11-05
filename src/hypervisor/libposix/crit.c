#include "config.h"
#include <hv.h>
#include "debug.h"
#include "crit.h"

unsigned int __crit_enter(void)
{
	unsigned int ret = sys_fast_prio_set(__CRIT_PRIO_MAX);
	assert(ret < __CRIT_PRIO_MAX);	// Nesting of critical sections is not allowed.
	return ret;
}

void __crit_leave(unsigned int oldprio)
{
	assert(sys_fast_prio_get() == __CRIT_PRIO_MAX);
	sys_fast_prio_set(oldprio);
}

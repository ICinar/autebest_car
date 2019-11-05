/*
 * time.c
 *
 * ARINC time API.
 *
 * azuepke, 2014-09-08: initial
 * azuepke, 2014-09-21: API complete
 */

#include "apex.h"


void TIMED_WAIT (
/*in */ SYSTEM_TIME_TYPE DELAY_TIME,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	unsigned int err;

	if (__apex_preemption_disabled() || __apex_is_error_handler()) {
		err = INVALID_MODE;
	} else if (DELAY_TIME == 0) {
		/* just yield */
		sys_yield();
		err = NO_ERROR;
	} else if (DELAY_TIME > 0) {
		/* normal timeout */
		err = sys_sleep(DELAY_TIME);
		assert(err == E_OS_TIMEOUT);
		err = NO_ERROR;
	} else {
		assert(DELAY_TIME < 0);
		/* infinite timeout not allowed here */
		err = INVALID_PARAM;
	}

	*RETURN_CODE = err;
}

void PERIODIC_WAIT (
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	unsigned int err;

	if (__apex_preemption_disabled() || __apex_is_error_handler()) {
		err = INVALID_MODE;
	} else {
		err = sys_wait_periodic();
		if (err == E_OS_TIMEOUT) {
			err = NO_ERROR;
		}
	}

	*RETURN_CODE = err;
}

void GET_TIME (
/*out*/ SYSTEM_TIME_TYPE *SYSTEM_TIME,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	time_t now;

	now = sys_gettime();

	*SYSTEM_TIME = now;
	*RETURN_CODE = NO_ERROR;
}

void REPLENISH (
/*in */ SYSTEM_TIME_TYPE BUDGET_TIME,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	unsigned int err;

	if (__apex_is_error_handler() || !__apex_in_normal_mode()) {
		err = NO_ACTION;
	} else {
		err = sys_replenish(BUDGET_TIME);
	}

	*RETURN_CODE = err;
}

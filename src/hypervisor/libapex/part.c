/*
 * part.c
 *
 * ARINC partitioning API.
 *
 * azuepke, 2014-09-08: initial
 * azuepke, 2014-09-26: partition API complete
 */

#include "apex.h"


void GET_PARTITION_STATUS (
/*out*/ PARTITION_STATUS_TYPE *PARTITION_STATUS,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	PARTITION_STATUS->PERIOD = __apex_part_period;
	PARTITION_STATUS->DURATION = __apex_part_duration;
	PARTITION_STATUS->IDENTIFIER = sys_part_self();
	PARTITION_STATUS->LOCK_LEVEL = __apex_lock_level;
	PARTITION_STATUS->OPERATING_MODE = __apex_operating_mode;
	PARTITION_STATUS->START_CONDITION = __apex_start_condition;

	*RETURN_CODE = NO_ERROR;
}

void SET_PARTITION_MODE (
/*in */ OPERATING_MODE_TYPE OPERATING_MODE,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	unsigned int err;

	if (__apex_is_init_hook() && (OPERATING_MODE == NORMAL)) {
		/* entering NORMAL mode, reset lock level */
		assert(__apex_lock_level == 1);
		__apex_lock_level = 0;
		__apex_operating_mode = NORMAL;
	}

	err = sys_part_set_operating_mode(OPERATING_MODE);

	*RETURN_CODE = err;
}

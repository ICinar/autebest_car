/*
 * sem.c
 *
 * ARINC semaphores.
 *
 * azuepke, 2014-09-08: initial
 */

#include "apex.h"

void CREATE_SEMAPHORE (
/*in */ SEMAPHORE_NAME_TYPE SEMAPHORE_NAME,
/*in */ SEMAPHORE_VALUE_TYPE CURRENT_VALUE,
/*in */ SEMAPHORE_VALUE_TYPE MAXIMUM_VALUE,
/*in */ QUEUING_DISCIPLINE_TYPE QUEUING_DISCIPLINE,
/*out*/ SEMAPHORE_ID_TYPE *SEMAPHORE_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	unsigned int disc;
	unsigned int err;
	unsigned int id;

	if (__apex_in_normal_mode()) {
		*RETURN_CODE = INVALID_MODE;
		return;
	}

	/* running in init hoook, no internal locking required */
	assert(__apex_is_init_hook());

	id = __apex_sem_find(SEMAPHORE_NAME);
	if (id == INVALID_ID) {
		*RETURN_CODE = INVALID_CONFIG;
		return;
	}

	if (__apex_sem_dyn[id].disc_max != SEM_INVALID) {
		*RETURN_CODE = NO_ACTION;
		return;
	}

	if ((MAXIMUM_VALUE <= 0) || (MAXIMUM_VALUE >= MAX_SEMAPHORE_VALUE)) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	if ((CURRENT_VALUE < 0) || (CURRENT_VALUE > MAXIMUM_VALUE)) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	if ((QUEUING_DISCIPLINE != FIFO) && (QUEUING_DISCIPLINE != PRIORITY)) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	disc = QUEUING_DISCIPLINE == PRIORITY ? WQ_DISCIPLINE_PRIO : WQ_DISCIPLINE_FIFO;
	err = sys_wq_set_discipline(__apex_sem_cfg[id].wq_id, disc, (uint32_t*)&__apex_sem_dyn[id]);
	if (err != E_OK) {
		*RETURN_CODE = INVALID_CONFIG;
		return;
	}

	__apex_sem_dyn[id].disc_max = MAXIMUM_VALUE;
	if (QUEUING_DISCIPLINE == PRIORITY) {
		__apex_sem_dyn[id].disc_max |= SEM_DISC_PRIO;
	}
	__apex_sem_dyn[id].val = CURRENT_VALUE;

	*SEMAPHORE_ID = id;
	*RETURN_CODE = NO_ERROR;
}

void WAIT_SEMAPHORE (
/*in */ SEMAPHORE_ID_TYPE SEMAPHORE_ID,
/*in */ SYSTEM_TIME_TYPE TIME_OUT,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	unsigned int err;
	unsigned int id;
	uint32_t compare;

	id = (unsigned int)SEMAPHORE_ID;
	if (id >= __apex_num_sems) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	if (__apex_sem_dyn[id].disc_max == SEM_INVALID) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	__apex_lock();

	if (__apex_sem_dyn[id].val > 0) {
		__apex_sem_dyn[id].val--;
		err = NO_ERROR;
	} else {
		/* have to wait ... */
		if (TIME_OUT == 0) {
			err = NOT_AVAILABLE;
		} else {
			__apex_sem_dyn[id].val--;
			compare = *((uint32_t*)&__apex_sem_dyn[id]);
			err = sys_wq_wait(__apex_sem_cfg[id].wq_id, compare, TIME_OUT,
							  __apex_proc_prio[__sys_sched_state.taskid]);
			assert((err == E_OK) || (err == E_OS_TIMEOUT));
			if (err != E_OK) {
				__apex_sem_dyn[id].val++;
				err = TIMED_OUT;
			}
		}
	}

	__apex_unlock();
	// FIXME: need to check for suspension!

	*RETURN_CODE = err;
}

void SIGNAL_SEMAPHORE (
/*in */ SEMAPHORE_ID_TYPE SEMAPHORE_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	unsigned int err;
	unsigned int id;
	uint16_t max;

	id = (unsigned int)SEMAPHORE_ID;
	if (id >= __apex_num_sems) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	if (__apex_sem_dyn[id].disc_max == SEM_INVALID) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	max = __apex_sem_dyn[id].disc_max & SEM_MAX_MASK;

	__apex_lock();

	if (__apex_sem_dyn[id].val >= max) {
		err = NO_ACTION;
	} else {
		__apex_sem_dyn[id].val++;
		if (__apex_sem_dyn[id].val <= 0) {
			/* wake one */
			err = sys_wq_wake(__apex_sem_cfg[id].wq_id, 1);
			assert(err == E_OK);
		}
		err = NO_ERROR;
	}

	__apex_unlock();

	*RETURN_CODE = err;
}

void GET_SEMAPHORE_ID (
/*in */ SEMAPHORE_NAME_TYPE SEMAPHORE_NAME,
/*out*/ SEMAPHORE_ID_TYPE *SEMAPHORE_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	unsigned int id;

	id = __apex_sem_find(SEMAPHORE_NAME);

	if (id == INVALID_ID) {
		*RETURN_CODE = INVALID_CONFIG;
		return;
	}

	if (__apex_sem_dyn[id].disc_max == SEM_INVALID) {
		*RETURN_CODE = INVALID_CONFIG;
		return;
	}

	*SEMAPHORE_ID = id;
	*RETURN_CODE = NO_ERROR;
}

void GET_SEMAPHORE_STATUS (
/*in */ SEMAPHORE_ID_TYPE SEMAPHORE_ID,
/*out*/ SEMAPHORE_STATUS_TYPE *SEMAPHORE_STATUS,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	unsigned int id;
	int16_t val;

	id = (unsigned int)SEMAPHORE_ID;
	if (id >= __apex_num_sems) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	if (__apex_sem_dyn[id].disc_max == SEM_INVALID) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	/* dynamic attributes only */
	val = __apex_sem_dyn[id].val;
	SEMAPHORE_STATUS->CURRENT_VALUE = val > 0 ? val : 0;
	SEMAPHORE_STATUS->MAXIMUM_VALUE = __apex_sem_dyn[id].disc_max & SEM_MAX_MASK;
	SEMAPHORE_STATUS->WAITING_PROCESSES = val < 0 ? -val : 0;

	*RETURN_CODE = NO_ERROR;
}

/*
 * process.c
 *
 * ARINC process API.
 *
 * azuepke, 2014-09-08: initial
 * azuepke, 2014-09-26: internal locking and most of the calls
 */

#include "apex.h"


void CREATE_PROCESS (
/*in */ PROCESS_ATTRIBUTE_TYPE *ATTRIBUTES,
/*out*/ PROCESS_ID_TYPE *PROCESS_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	const struct apex_proc_cfg *cfg;
	unsigned int id;

	if (__apex_in_normal_mode()) {
		*RETURN_CODE = INVALID_MODE;
		return;
	}

	/* running in init hoook, no internal locking required */
	assert(__apex_is_init_hook());

	id = __apex_proc_find(ATTRIBUTES->NAME);
	if (id == INVALID_ID) {
		*RETURN_CODE = INVALID_CONFIG;
		return;
	}

	if (__apex_proc_state[id] != PROC_STATE_DEAD) {
		*RETURN_CODE = NO_ACTION;
		return;
	}

	cfg = &__apex_proc_cfg[id];

	if (ATTRIBUTES->STACK_SIZE > cfg->stack_size) {
		*RETURN_CODE = INVALID_CONFIG;
		return;
	}
	if ((uint32_t)ATTRIBUTES->ENTRY_POINT != cfg->entry_point) {
		__apex_panic("CREATE_PROCESS entry point");
	}
	if (ATTRIBUTES->BASE_PRIORITY != cfg->base_prio) {
		__apex_panic("CREATE_PROCESS base prio");
	}
	if (ATTRIBUTES->PERIOD != (timeout_t)cfg->period) {
		__apex_panic("CREATE_PROCESS period");
		// FIXME: return INVALID_PARAM / INVALID_CONFIG
	}
	if (ATTRIBUTES->TIME_CAPACITY != cfg->capacity) {
		__apex_panic("CREATE_PROCESS capacity");
		// FIXME: return INVALID_PARAM / INVALID_CONFIG
	}

	__apex_proc_state[id] = PROC_STATE_DORMANT;
	__apex_proc_prio[id] = cfg->base_prio;

	*PROCESS_ID = id;
	*RETURN_CODE = NO_ERROR;
}

void SET_PRIORITY (
/*in */ PROCESS_ID_TYPE PROCESS_ID,
/*in */ PRIORITY_TYPE PRIO,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	unsigned int err;
	unsigned int id;

	id = (unsigned int)PROCESS_ID;
	if (id >= __apex_num_procs) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	if (__apex_proc_state[id] == PROC_STATE_DEAD) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	if (PRIO > __apex_part_max_prio) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	__apex_lock();

	if (__apex_proc_state[id] == PROC_STATE_DORMANT) {
		err = INVALID_MODE;
		goto out;
	}

	/* don't change priority of processes in critical sections */
	if ((__apex_proc_state[id] == PROC_STATE_NORMAL) && !__apex_is_self(id)) {
		err = sys_task_set_prio(id, PRIO);
		if (err != E_OK) {
			err = INVALID_MODE;
			goto out;
		}
	}
	__apex_proc_prio[id] = PRIO;

	err = NO_ERROR;

out:
	__apex_unlock();

	*RETURN_CODE = err;
}

void SUSPEND_SELF (
/*in */ SYSTEM_TIME_TYPE TIME_OUT,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	const struct apex_proc_cfg *cfg;
	unsigned int err;
	unsigned int id;

	if (__apex_preemption_disabled() || __apex_is_error_handler()) {
		*RETURN_CODE = INVALID_MODE;
		return;
	}

	id = __sys_sched_state.taskid;

	cfg = &__apex_proc_cfg[id];
	if (cfg->period > 0) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	if (TIME_OUT == 0) {
		*RETURN_CODE = NO_ERROR;
		return;
	}

	__apex_lock();

	__apex_proc_state[id] = PROC_STATE_SUSP_SLEEP;

	err = sys_sleep(TIME_OUT);
	assert((err == E_OK) || (err == E_OS_TIMEOUT));
	/* timeout may expire or we're woken up by sys_unblock() */
	if (err == E_OS_TIMEOUT) {
		err = TIMED_OUT;
	}

	if (__apex_proc_state[id] == PROC_STATE_SUSP_SLEEP) {
		__apex_proc_state[id] = PROC_STATE_NORMAL;
	}

	__apex_unlock_yield();

	*RETURN_CODE = err;
}

void SUSPEND (
/*in */ PROCESS_ID_TYPE PROCESS_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	const struct apex_proc_cfg *cfg;
	unsigned int err;
	unsigned int id;

	id = (unsigned int)PROCESS_ID;
	if ((id >= __apex_num_procs) || __apex_is_self(id)) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	if (__apex_proc_state[id] == PROC_STATE_DEAD) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	cfg = &__apex_proc_cfg[id];
	if (cfg->period > 0) {
		*RETURN_CODE = INVALID_MODE;
		return;
	}

	if (__apex_is_error_handler() &&
	    (id == __apex_previous_process) &&
	    (__apex_lock_level > 0)) {
		*RETURN_CODE = INVALID_MODE;
		return;
	}

	__apex_lock();

	switch (__apex_proc_state[id]) {
	case PROC_STATE_NORMAL:
		/* task not in critical section */
		__apex_proc_state[id] = PROC_STATE_SUSP_PRIO0;
		err = sys_task_set_prio(id, 0);
		assert(err == E_OK);
		/* ignore repairing of crit state, task is DORMANT */
		break;

	case PROC_STATE_CRIT:
		/* task in critical section, let task suspend self */
		__apex_proc_state[id] = PROC_STATE_CRIT_SUSP;
		err = NO_ERROR;
		break;

	case PROC_STATE_CRIT_SUSP:
	case PROC_STATE_SUSP_SLEEP:
	case PROC_STATE_SUSP_PRIO0:
		err = NO_ACTION;
		break;

	case PROC_STATE_DORMANT:
	default:
		err = INVALID_MODE;
		break;
	}

	__apex_unlock();

	*RETURN_CODE = err;
}

void RESUME (
/*in */ PROCESS_ID_TYPE PROCESS_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	unsigned int err;
	unsigned int id;

	id = (unsigned int)PROCESS_ID;
	if ((id >= __apex_num_procs) || __apex_is_self(id)) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	if (__apex_proc_state[id] == PROC_STATE_DEAD) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	__apex_lock();

	switch (__apex_proc_state[id]) {
	case PROC_STATE_NORMAL:
	case PROC_STATE_CRIT:
		*RETURN_CODE = NO_ACTION;
		return;

	case PROC_STATE_CRIT_SUSP:
		__apex_proc_state[id] = PROC_STATE_CRIT;
		err = NO_ERROR;
		break;

	case PROC_STATE_SUSP_SLEEP:
		__apex_proc_state[id] = PROC_STATE_NORMAL;
		err = sys_unblock(id);
		/* task may have expired in this moment, ignore error */
		err = NO_ERROR;
		break;

	case PROC_STATE_SUSP_PRIO0:
		__apex_proc_state[id] = PROC_STATE_NORMAL;
		err = sys_task_set_prio(id, __apex_proc_prio[id]);
		assert(err == E_OK);
		break;

	case PROC_STATE_DORMANT:
	default:
		*RETURN_CODE = INVALID_MODE;
		return;
	}

	__apex_unlock();

	*RETURN_CODE = err;
}

void STOP_SELF (void)
{
	unsigned int id;

	if (!__apex_is_error_handler()) {
		__apex_lock_level = 0;
	}

	id = __sys_sched_state.taskid;

	__apex_lock();

	__apex_proc_state[id] = PROC_STATE_DORMANT;

	sys_task_terminate();
	assert(0);
}

void STOP (
/*in */ PROCESS_ID_TYPE PROCESS_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	unsigned int err;
	unsigned int id;

	id = (unsigned int)PROCESS_ID;
	if (id >= __apex_num_procs) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	if (__apex_proc_state[id] == PROC_STATE_DEAD) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	__apex_lock();

	if (__apex_proc_state[id] != PROC_STATE_DORMANT) {
		if (__apex_is_error_handler() && (id == __apex_previous_process)) {
			__apex_lock_level = 0;
		}

		__apex_proc_state[id] = PROC_STATE_DORMANT;
		err = sys_task_terminate_other(id);
	} else {
		err = NO_ACTION;
	}

	__apex_unlock();

	*RETURN_CODE = err;
}

void START (
/*in */ PROCESS_ID_TYPE PROCESS_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	const struct apex_proc_cfg *cfg;
	unsigned int err;
	unsigned int id;

	id = (unsigned int)PROCESS_ID;
	if (id >= __apex_num_procs) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	if (__apex_proc_state[id] == PROC_STATE_DEAD) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	cfg = &__apex_proc_cfg[id];

	__apex_lock();

	err = sys_task_activate(id);

	if (err == E_OK) {
		__apex_proc_state[id] = PROC_STATE_NORMAL;
		__apex_proc_prio[id] = cfg->base_prio;
	} else {
		err = NO_ACTION;
	}

	__apex_unlock();

	*RETURN_CODE = err;
}

void DELAYED_START (
/*in */ PROCESS_ID_TYPE PROCESS_ID,
/*in */ SYSTEM_TIME_TYPE DELAY_TIME,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	const struct apex_proc_cfg *cfg;
	unsigned int err;
	unsigned int id;
	int sync;

	id = (unsigned int)PROCESS_ID;
	if (id >= __apex_num_procs) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	if (__apex_proc_state[id] == PROC_STATE_DEAD) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	cfg = &__apex_proc_cfg[id];

	__apex_lock();

	/* delay start until the partition enters normal mode */
	sync = !__apex_in_normal_mode();
	err = sys_task_delayed_activate(id, sync, DELAY_TIME);

	if (err == E_OK) {
		__apex_proc_state[id] = PROC_STATE_NORMAL;
		__apex_proc_prio[id] = cfg->base_prio;
	}

	__apex_unlock();

	*RETURN_CODE = err;
}

void LOCK_PREEMPTION (
/*out*/ LOCK_LEVEL_TYPE *LOCK_LEVEL,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	if (__apex_is_error_handler() || !__apex_in_normal_mode()) {
		*RETURN_CODE = NO_ACTION;
		return;
	}
	if (__apex_lock_level >= MAX_LOCK_LEVEL) {
		*RETURN_CODE = INVALID_CONFIG;
		return;
	}

	if (__apex_lock_level == 0) {
		__apex_lock();
	}
	__apex_lock_level++;

	*LOCK_LEVEL = __apex_lock_level;
	*RETURN_CODE = NO_ERROR;
}

void UNLOCK_PREEMPTION (
/*out*/ LOCK_LEVEL_TYPE *LOCK_LEVEL,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	if (__apex_is_error_handler() || !__apex_in_normal_mode()) {
		*RETURN_CODE = NO_ACTION;
		return;
	}
	if (__apex_lock_level == 0) {
		*RETURN_CODE = NO_ACTION;
		return;
	}

	__apex_lock_level--;
	if (__apex_lock_level == 0) {
		__apex_unlock();
	}

	*LOCK_LEVEL = __apex_lock_level;
	*RETURN_CODE = NO_ERROR;
}

void GET_MY_ID (
/*out*/ PROCESS_ID_TYPE *PROCESS_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	unsigned int id;

	id = __sys_sched_state.taskid;

	if ((id == __apex_init_hook_id) ||
	    (id == __apex_error_handler_hook_id)) {
		*RETURN_CODE = INVALID_MODE;
	} else {
		*RETURN_CODE = NO_ERROR;
	}

	*PROCESS_ID = id;
}

void GET_PROCESS_ID (
/*in */ PROCESS_NAME_TYPE PROCESS_NAME,
/*out*/ PROCESS_ID_TYPE *PROCESS_ID,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	unsigned int id;

	id = __apex_proc_find(PROCESS_NAME);

	if (id == INVALID_ID) {
		*RETURN_CODE = INVALID_CONFIG;
		return;
	}

	if (__apex_proc_state[id] == PROC_STATE_DEAD) {
		*RETURN_CODE = INVALID_CONFIG;
		return;
	}

	*PROCESS_ID = id;
	*RETURN_CODE = NO_ERROR;
}

void GET_PROCESS_STATUS (
/*in */ PROCESS_ID_TYPE PROCESS_ID,
/*out*/ PROCESS_STATUS_TYPE *PROCESS_STATUS,
/*out*/ RETURN_CODE_TYPE *RETURN_CODE )
{
	const struct apex_proc_cfg *cfg;
	unsigned int state;
	unsigned int err;
	unsigned int id;

	id = (unsigned int)PROCESS_ID;
	if (id >= __apex_num_procs) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	if (__apex_proc_state[id] == PROC_STATE_DEAD) {
		*RETURN_CODE = INVALID_PARAM;
		return;
	}

	/* static attributes */
	cfg = &__apex_proc_cfg[id];
	PROCESS_STATUS->ATTRIBUTES.PERIOD = cfg->period;
	PROCESS_STATUS->ATTRIBUTES.TIME_CAPACITY = cfg->capacity;
	PROCESS_STATUS->ATTRIBUTES.ENTRY_POINT = (void*)cfg->entry_point;
	PROCESS_STATUS->ATTRIBUTES.STACK_SIZE = cfg->stack_size;
	PROCESS_STATUS->ATTRIBUTES.BASE_PRIORITY = cfg->base_prio;
	PROCESS_STATUS->ATTRIBUTES.DEADLINE = cfg->deadline_type;
	__apex_name_cpy(PROCESS_STATUS->ATTRIBUTES.NAME, cfg->name);

	/* get process state and translate */
	err = sys_task_get_state(id, &state);
	assert(err == E_OK);
	(void)err;

	switch (state) {
	case TASK_STATE_RUNNING:
		state = RUNNING;
		break;

	case TASK_STATE_READY:
		state = READY;
		break;

	case TASK_STATE_SUSPENDED:
		state = DORMANT;
		break;

	default:
		/* NOTE: this covers all waiting states of the kernel, e.g.
		 * TASK_STATE_WAIT_EV, TASK_STATE_WAIT_WQ, TASK_STATE_WAIT_ACT,
		 * TASK_STATE_WAIT_SEND, and TASK_STATE_WAIT_SEND.
		 */
		state = WAITING;
		break;
	}

	/* dynamic attributes */
	PROCESS_STATUS->DEADLINE_TIME = 42;	// FIXME -> need to retrieve from kernel
	PROCESS_STATUS->CURRENT_PRIORITY = __apex_proc_prio[id];
	PROCESS_STATUS->PROCESS_STATE = state;

	*RETURN_CODE = NO_ERROR;
}

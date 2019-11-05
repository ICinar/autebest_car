/*
 * alarm.c
 *
 * OSEK alarms
 *
 * azuepke, 2014-06-05: initial
 */

#include <kernel.h>
#include <assert.h>
#include <alarm.h>
#include <counter.h>
#include <hv_error.h>
#include <sched.h>
#include <part.h>
#include <task.h>
#include <event.h>
#include <schedtab.h>
#include <hm.h>
#include <ipi.h>

/** remove an alarm from counter's queue -- internal routine */
static inline void alarm_remove(struct alarm *alm, const struct counter_cfg *ctr_cfg);


__init static inline void alarm_init_one(
	const struct alarm_cfg *alm_cfg,
	struct alarm *alm,
	unsigned int global_alarm_id)
{
#ifndef NDEBUG
	switch (alm_cfg->action) {
	case ALARM_ACTION_TASK:
	case ALARM_ACTION_HOOK:
	case ALARM_ACTION_EVENT:
		assert(alm_cfg->u.task != NULL);
		assert(alm_cfg->event_bit < 32);
		break;
	case ALARM_ACTION_INVOKE:
		assert(alm_cfg->u.alarm_callback != NULL);
		break;
	case ALARM_ACTION_COUNTER:
		assert(alm_cfg->u.counter_cfg != NULL);
		break;
	case ALARM_ACTION_SCHEDTAB:
		assert(alm_cfg->u.schedtab != NULL);
		break;
	default:
		assert(0);
		break;
	}
#endif

	alm->next = NULL;
	alm->expiry = 0;
	alm->cycle = 0;

	alm->alarm_id = global_alarm_id;
	assert(alm_cfg->counter_id < num_counters);
	assert(alm_cfg->cpu_id == counter_cfg[alm_cfg->counter_id].cpu_id);
	alm->counter_id = alm_cfg->counter_id;
	alm->state = ALARM_STATE_IDLE;
}

__init void alarm_init_all(void)
{
	const struct schedtab_cfg *st_cfg;
	const struct part_cfg *part_cfg;
	const struct alarm_cfg *alm_cfg;
	unsigned int global_alarm_id;
	unsigned int i, j;
	struct alarm *alm;

	/* initialize all alarms (just partly) */
	/* there's a per-partition alarm initializer later */
	global_alarm_id = 0;
	for (i = 0; i < num_partitions; i++) {
		part_cfg = part_get_part_cfg(i);

		for (j = 0; j < part_cfg->num_alarms; j++) {

			alm = &part_cfg->alarms[j];
			assert(alm != NULL);

			alm_cfg = &alarm_cfg[global_alarm_id];
			alarm_init_one(alm_cfg, alm, global_alarm_id);

			global_alarm_id++;
		}
	}

	for (i = 0; i < num_schedtabs; i++) {
		st_cfg = &schedtab_cfg[i];

		alm = st_cfg->alarm;
		assert(alm != NULL);

		alm_cfg = &alarm_cfg[global_alarm_id];
		alarm_init_one(alm_cfg, alm, global_alarm_id);

		global_alarm_id++;
	}
}

/** Get static configuration of the alarm */
void sys_alarm_base(unsigned int alarm_id)
{
	const struct counter_cfg *ctr_cfg;
	const struct alarm_cfg *alm_cfg;
	const struct part_cfg *part_cfg;
	struct alarm *alm;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);

	/* validate alarm ID in this partition and get corresponding counter */
	if (alarm_id >= part_cfg->num_alarms) {
		SET_RET(E_OS_ID);
		return;
	}
	alm = &part_cfg->alarms[alarm_id];
	alm_cfg = &alarm_cfg[alm->alarm_id];
	ctr_cfg = &counter_cfg[alm_cfg->counter_id];

	SET_OUT1(ctr_cfg->maxallowedvalue);
	SET_OUT2(ctr_cfg->ticksperbase);
	SET_OUT3(ctr_cfg->mincycle);
	SET_RET(E_OK);
}

/** Get relative number of ticks before the alarm expires */
void sys_alarm_get(unsigned int alarm_id)
{
	const struct counter_cfg *ctr_cfg;
	const struct part_cfg *part_cfg;
	struct alarm *alm;
	ctrtick_t remaining;
	ctrtick_t current;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);

	/* validate alarm ID in this partition and get corresponding counter */
	if (alarm_id >= part_cfg->num_alarms) {
		SET_RET(E_OS_ID);
		return;
	}
	alm = &part_cfg->alarms[alarm_id];

	if (alm->state != ALARM_STATE_ACTIVE) {
		SET_RET(E_OS_NOFUNC);
		return;
	}

	/* get current counter value */
	ctr_cfg = &counter_cfg[alm->counter_id];
	current = counter_query(ctr_cfg);

	remaining = ctr_diff(current, alm->expiry, ctr_cfg->maxallowedvalue);

	SET_OUT1(remaining);
	SET_RET(E_OK);
}

/** Set relative number of ticks before the alarm expires */
void sys_alarm_set_rel(unsigned int alarm_id, ctrtick_t increment, ctrtick_t cycle)
{
	const struct counter_cfg *ctr_cfg;
	const struct part_cfg *part_cfg;
	struct alarm *alm;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);

	/* validate alarm ID in this partition and get corresponding counter */
	if (alarm_id >= part_cfg->num_alarms) {
		SET_RET(E_OS_ID);
		return;
	}
	alm = &part_cfg->alarms[alarm_id];

	if (alm->state == ALARM_STATE_ACTIVE) {
		SET_RET(E_OS_STATE);
		return;
	}

	/* validate increment and cycle */
	ctr_cfg = &counter_cfg[alm->counter_id];
	if ((increment == 0) || (increment > ctr_cfg->maxallowedvalue)) {
		SET_RET(E_OS_VALUE);
		return;
	}
	if ((cycle != 0) && (cycle < ctr_cfg->mincycle)) {
		SET_RET(E_OS_VALUE);
		return;
	}
	if (cycle > ctr_cfg->maxallowedvalue) {
		SET_RET(E_OS_VALUE);
		return;
	}

	SET_RET(E_OK);

	alarm_set_rel(alm, ctr_cfg, increment, cycle);
}

/** set alarm to relative time value "increment" */
void alarm_set_rel(struct alarm *alm, const struct counter_cfg *ctr_cfg, ctrtick_t increment, ctrtick_t cycle)
{
	ctrtick_t current;
	ctrtick_t expiry;

	assert(alm != NULL);
	assert(ctr_cfg != NULL);

	/* get current counter value */
	current = counter_query(ctr_cfg);

	expiry = ctr_add(current, increment, ctr_cfg->maxallowedvalue);

	/* enqueue alarm */
	alm->expiry = expiry;
	alm->cycle = cycle;
	alarm_enqueue(alm, ctr_cfg);

	if (ctr_cfg->type == COUNTER_TYPE_HW) {
		counter_change_hw(ctr_cfg);
	}
}

/** Set absolute number of ticks at which the alarm expires */
void sys_alarm_set_abs(unsigned int alarm_id, ctrtick_t expiry, ctrtick_t cycle)
{
	const struct counter_cfg *ctr_cfg;
	const struct part_cfg *part_cfg;
	struct alarm *alm;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);

	/* validate alarm ID in this partition and get corresponding counter */
	if (alarm_id >= part_cfg->num_alarms) {
		SET_RET(E_OS_ID);
		return;
	}
	alm = &part_cfg->alarms[alarm_id];

	if (alm->state == ALARM_STATE_ACTIVE) {
		SET_RET(E_OS_STATE);
		return;
	}

	/* validate expiry and cycle */
	ctr_cfg = &counter_cfg[alm->counter_id];
	if (expiry > ctr_cfg->maxallowedvalue) {
		SET_RET(E_OS_VALUE);
		return;
	}
	if ((cycle != 0) && (cycle < ctr_cfg->mincycle)) {
		SET_RET(E_OS_VALUE);
		return;
	}
	if (cycle > ctr_cfg->maxallowedvalue) {
		SET_RET(E_OS_VALUE);
		return;
	}

	SET_RET(E_OK);

	alarm_set_abs(alm, ctr_cfg, expiry, cycle);
}

/** set alarm to absolute time value "expiry" */
void alarm_set_abs(struct alarm *alm, const struct counter_cfg *ctr_cfg, ctrtick_t expiry, ctrtick_t cycle)
{
	assert(alm != NULL);
	assert(ctr_cfg != NULL);

	/* enqueue alarm */
	alm->expiry = expiry;
	alm->cycle = cycle;
	alarm_enqueue(alm, ctr_cfg);

	if (ctr_cfg->type == COUNTER_TYPE_HW) {
		counter_change_hw(ctr_cfg);
	}
}

/** Cancel an alarm (before it expires) */
void sys_alarm_cancel(unsigned int alarm_id)
{
	const struct part_cfg *part_cfg;
	struct alarm *alm;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);

	/* validate alarm ID in this partition and get corresponding counter */
	if (alarm_id >= part_cfg->num_alarms) {
		SET_RET(E_OS_ID);
		return;
	}
	alm = &part_cfg->alarms[alarm_id];

	if (alm->state != ALARM_STATE_ACTIVE) {
		SET_RET(E_OS_NOFUNC);
		return;
	}

	SET_RET(E_OK);

	alarm_cancel(alm);
}

/** cancel alarm and notify counter */
void alarm_cancel(struct alarm *alm)
{
	const struct counter_cfg *ctr_cfg;

	assert(alm != NULL);
	assert(alm->state == ALARM_STATE_ACTIVE);

	ctr_cfg = &counter_cfg[alm->counter_id];

	alarm_remove(alm, ctr_cfg);

	if (ctr_cfg->type == COUNTER_TYPE_HW) {
		counter_change_hw(ctr_cfg);
	}
}

/** enqueue alarm */
void alarm_enqueue(struct alarm *alm, const struct counter_cfg *ctr_cfg)
{
	struct alarm **next_p;
	struct counter *ctr;
	ctrtick_t his_diff;
	ctrtick_t my_diff;

	assert(alm != NULL);
	assert(ctr_cfg != NULL);
	ctr = ctr_cfg->counter;

	assert(alm->state == ALARM_STATE_IDLE);
	alm->state = ALARM_STATE_ACTIVE;

	assert(alm->next == NULL);
	/* NOTE: only the debug version clears it to NULL */
	alm->next = NULL;

	my_diff = ctr_diff(ctr->current, alm->expiry, ctr_cfg->maxallowedvalue);

	/* sorted insert into ctr's list */
	next_p = &ctr->first;
	while (*next_p != NULL) {
		his_diff = ctr_diff(ctr->current, (*next_p)->expiry, ctr_cfg->maxallowedvalue);
		if (his_diff > my_diff) {
			/* insert before */
			alm->next = *next_p;
			*next_p = alm;
			return;
		}
		next_p = &(*next_p)->next;
	}

	/* insert at end */
	assert(*next_p == NULL);
	*next_p = alm;
}

/** internal routine to let an alarm expire immediately */
void alarm_enqueue_first(struct alarm *alm)
{
	const struct counter_cfg *ctr_cfg;
	struct counter *ctr;

	assert(alm != NULL);

	ctr_cfg = &counter_cfg[alm->counter_id];
	ctr = ctr_cfg->counter;

	/* fake alarm to expire now */
	assert(alm->state == ALARM_STATE_IDLE);
	alm->state = ALARM_STATE_ACTIVE;
	alm->expiry = ctr->current;
	alm->cycle = 0;

	/* enqueue alarm at beginning of counter's queue */
	assert(alm->next == NULL);
	alm->next = ctr->first;
	ctr->first = alm;
}


/** expire an alarm */
void alarm_expire(struct alarm *alm)
{
	const struct alarm_cfg *alm_cfg;
	unsigned int err;
	evmask_t mask;

	assert(alm != NULL);

	assert(alm->state == ALARM_STATE_ACTIVE);
	alm->state = ALARM_STATE_IDLE;
#ifndef NDEBUG
	alm->next = NULL;
#endif

	alm_cfg = &alarm_cfg[alm->alarm_id];

	switch (alm_cfg->action) {
	case ALARM_ACTION_EVENT:
#ifdef SMP
		if (alm_cfg->u.task->cfg->cpu_id != arch_cpu_id()) {
			ipi_enqueue(alm_cfg->u.task->cfg->cpu_id, alm_cfg->u.task, IPI_ACTION_EVENT, alm_cfg->event_bit);
			break;
		}
#endif
		mask = 1u << alm_cfg->event_bit;
		err = ev_set(alm_cfg->u.task, mask);
		if (unlikely(err != E_OK)) {
			assert(err == E_OS_STATE);
			hm_async_task_error(alm_cfg->u.task->cfg, HM_ERROR_TASK_STATE_ERROR, alm_cfg->event_bit);
		}
		break;

	case ALARM_ACTION_TASK:
#ifdef SMP
		if (alm_cfg->u.task->cfg->cpu_id != arch_cpu_id()) {
			ipi_enqueue(alm_cfg->u.task->cfg->cpu_id, alm_cfg->u.task, IPI_ACTION_TASK, 0);
			break;
		}
#endif
		err = task_check_activate(alm_cfg->u.task);
		if (err == E_OK) {
			task_do_activate(alm_cfg->u.task);
		} else {
			assert(err == E_OS_LIMIT);
			hm_async_task_error(alm_cfg->u.task->cfg, HM_ERROR_TASK_ACTIVATION_ERROR, 0);
		}
		break;

	case ALARM_ACTION_HOOK:
#ifdef SMP
		if (alm_cfg->u.task->cfg->cpu_id != arch_cpu_id()) {
			ipi_enqueue(alm_cfg->u.task->cfg->cpu_id, alm_cfg->u.task, IPI_ACTION_HOOK, 0);
			break;
		}
#endif
		err = task_check_activate(alm_cfg->u.task);
		if (err == E_OK) {
			task_do_activate(alm_cfg->u.task);
		} else {
			assert(err == E_OS_LIMIT);
			/* no error reported here */
		}
		break;

	case ALARM_ACTION_INVOKE:
		alm_cfg->u.alarm_callback();
		break;

	case ALARM_ACTION_COUNTER:
		/* NOTE: this is not required by AUTOSAR, but we use this mechanism
		 * to kick schedule tables in other partitions or on other cores!
		 */
		/* FIXME: this may lead to recursion: kernel_increment_counter()
		 * will call alarm_expire() again!
		 */
#ifdef SMP
		if (alm_cfg->u.counter_cfg->cpu_id != arch_cpu_id()) {
			/* cross-core counter */
			ipi_enqueue(alm_cfg->u.counter_cfg->cpu_id, alm_cfg->u.counter_cfg, IPI_ACTION_COUNTER, 0);
			break;
		}
#endif
		kernel_increment_counter(alm_cfg->u.counter_cfg, 1);
		break;

	case ALARM_ACTION_SCHEDTAB:
		schedtab_expire(alm, alm_cfg->u.schedtab);
		break;

	default:
		assert(0);
		break;
	}
}

/** remove an alarm from counter's queue -- internal routine */
static inline void alarm_remove(struct alarm *alm, const struct counter_cfg *ctr_cfg)
{
	struct alarm **next_p;
	struct counter *ctr;

	assert(alm != NULL);
	assert(alm->state == ALARM_STATE_ACTIVE);
	assert(ctr_cfg != NULL);
	ctr = ctr_cfg->counter;
	assert(ctr->first != NULL);

	/* iterate ctr's list and remove alm */
	next_p = &ctr->first;
	while (*next_p != alm) {
		next_p = &(*next_p)->next;
	}
	assert(next_p != NULL);
	assert(*next_p == alm);

	/* remove */
	*next_p = alm->next;
	alm->state = ALARM_STATE_IDLE;
#ifndef NDEBUG
	alm->next = NULL;
#endif
}

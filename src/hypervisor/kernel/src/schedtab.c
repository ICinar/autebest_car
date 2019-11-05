/*
 * schedtab.c
 *
 * Scheduling tables (schedtab)
 *
 * azuepke, 2014-06-10: initial
 */

#include <kernel.h>
#include <assert.h>
#include <schedtab.h>
#include <counter.h>
#include <alarm.h>
#include <sched.h>
#include <part.h>
#include <task.h>
#include <event.h>
#include <hv_error.h>
#include <hm.h>

__init void schedtab_init_all(void)
{
#ifndef NDEBUG
	const struct schedtab_cfg *cfg;
#endif
	const struct part_cfg *part_cfg;
	struct schedtab *schedtab;
	unsigned int global_schedtab_id;
	unsigned int i, j;

	global_schedtab_id = 0;
	for (i = 0; i < num_partitions; i++) {
		part_cfg = part_get_part_cfg(i);

		for (j = 0; j < part_cfg->num_schedtabs; j++) {
			schedtab = &part_cfg->schedtabs[j];
			assert(schedtab != NULL);

#ifndef NDEBUG
			cfg = &schedtab_cfg[global_schedtab_id];
			assert(cfg->counter_id < num_counters);
			assert(cfg->alarm != NULL);
			assert(cfg->duration > 0);
			assert(cfg->precision <= cfg->duration);
#endif

			schedtab->schedtab_id = global_schedtab_id;
			schedtab->state = SCHEDTAB_STATE_STOPPED;

			global_schedtab_id++;
		}
	}
	assert(global_schedtab_id == num_schedtabs);
}

static void schedtab_start(struct schedtab *schedtab, const struct schedtab_cfg *cfg)
{
	assert(schedtab != NULL);

	/* start schedule table */
	assert((schedtab->state == SCHEDTAB_STATE_STOPPED) ||
	       (schedtab->state == SCHEDTAB_STATE_NEXT) ||
	       (schedtab->state == SCHEDTAB_STATE_WAITING));
	if (cfg->flags & SCHEDTAB_FLAG_SYNC_IMPLICIT) {
		schedtab->state = SCHEDTAB_STATE_RUNNING_SYNC;
	} else {
		schedtab->state = SCHEDTAB_STATE_RUNNING;
	}
	if (cfg->flags & SCHEDTAB_FLAG_REPEATING) {
		schedtab->next = schedtab;
	} else {
		schedtab->next = NULL;
	}
	schedtab->current_idx = cfg->start_idx;

	/* counter is synchronized when schedule table starts */
	schedtab->deviation = 0;
}

void schedtab_expire(struct alarm *alm, struct schedtab *schedtab)
{
	const struct schedtab_action_cfg *action;
	unsigned int last_action;
	struct task *task;
	unsigned int err;
	evmask_t mask;

	ctrtick_t lengthen_time;
	ctrtick_t shorten_time;
	ctrtick_t wait_time;

	assert(alm != NULL);
	assert(schedtab != NULL);
	assert(schedtab->state == SCHEDTAB_STATE_RUNNING ||
	       schedtab->state == SCHEDTAB_STATE_RUNNING_SYNC ||
	       schedtab->state == SCHEDTAB_STATE_RUNNING_ASYNC);

scan_actions_again:
	lengthen_time = 0;
	shorten_time = 0;
	wait_time = 0;

	/* process schedule table action */
next_action:
	action = &schedtab_action_cfg[schedtab->current_idx];
	schedtab->current_idx++;
	last_action = action->action;
	switch (last_action) {
	case SCHEDTAB_ACTION_EVENT:
		task = action->u.task;
		mask = 1u << action->event_bit;
		err = ev_set(task, mask);
		if (unlikely(err != E_OK)) {
			assert(err == E_OS_STATE);
			hm_async_task_error(action->u.task->cfg, HM_ERROR_TASK_STATE_ERROR, action->event_bit);
		}
		goto next_action;

	case SCHEDTAB_ACTION_TASK:
		err = task_check_activate(action->u.task);
		if (err == E_OK) {
			task_do_activate(action->u.task);
		} else {
			assert(err == E_OS_LIMIT);
			hm_async_task_error(action->u.task->cfg, HM_ERROR_TASK_ACTIVATION_ERROR, 0);
		}
		goto next_action;

	case SCHEDTAB_ACTION_HOOK:
		err = task_check_activate(action->u.task);
		if (err == E_OK) {
			task_do_activate(action->u.task);
		} else {
			assert(err == E_OS_LIMIT);
			/* no error reported here */
		}
		goto next_action;

	case SCHEDTAB_ACTION_WAIT:
		wait_time = action->u.time;
		break;

	case SCHEDTAB_ACTION_SHORTEN:
		shorten_time = action->u.time;
		goto next_action;

	case SCHEDTAB_ACTION_LENGTHEN:
		lengthen_time = action->u.time;
		goto next_action;

	case SCHEDTAB_ACTION_WRAP:
		schedtab->current_idx = action->next_idx;
		break;

	case SCHEDTAB_ACTION_START:
		/* for synchronized tables, keep track of counter offset - as the
		 * counter may have a wider range than the schedule table, we have to
		 * do this at least once during each run. alternatively, we'd have to
		 * do modulo calculation in sys_schedtab_sync */
		schedtab->sync_counter_offset = counter_cfg[alm->counter_id].counter->current;
		goto next_action;
	}

	/* only WAIT and WRAP leave the loop */
	assert((last_action == SCHEDTAB_ACTION_WAIT) ||
	       (last_action == SCHEDTAB_ACTION_WRAP));

	/* handle wrap-around */
	if (last_action == SCHEDTAB_ACTION_WRAP) {
		const struct schedtab_cfg *next_cfg;
		struct schedtab *next_schedtab;
		struct alarm *next_alm;

		next_schedtab = schedtab->next;
		if (next_schedtab == NULL) {
			/* this schedule table stops */
			goto stop_this_schedtab;
		}

		if (next_schedtab == schedtab) {
			/* this schedule table repeats */
			goto scan_actions_again;
		}
		next_cfg = &schedtab_cfg[next_schedtab->schedtab_id];

		/* start the other schedule */
		assert(next_schedtab->state == SCHEDTAB_STATE_NEXT);
		schedtab_start(next_schedtab, next_cfg);
		/* transfer our (synchronous?) state and deviation */
		next_schedtab->state = schedtab->state;
		next_schedtab->deviation = schedtab->deviation;

		/* prepare its alarm */
		next_alm = next_cfg->alarm;
		assert(alm->state == ALARM_STATE_IDLE);
		alarm_enqueue_first(next_alm);
		goto stop_this_schedtab;
	}

	/* adjust deviation */
	if ((schedtab->deviation != 0) &&
	    (schedtab->state >= SCHEDTAB_STATE_RUNNING_SYNC)) {
		const struct schedtab_cfg *cfg;
		ctrtick_t abs_deviation;
		ctrtick_t adjust;

		if (schedtab->deviation < 0) {
			/* schedule table runs too slow */
			adjust = abs_deviation = -schedtab->deviation;
			if (adjust > shorten_time) {
				adjust = shorten_time;
			}

			wait_time -= adjust;
			schedtab->sync_counter_offset -= adjust;
			schedtab->deviation += adjust;
		} else /* schedtab->deviation > 0 */ {
			/* schedule table runs too fast */
			adjust = abs_deviation = schedtab->deviation;
			if (adjust > lengthen_time) {
				adjust = lengthen_time;
			}

			wait_time += adjust;
			schedtab->sync_counter_offset += adjust;
			schedtab->deviation -= adjust;
		}
		abs_deviation -= adjust;

		cfg = &schedtab_cfg[schedtab->schedtab_id];
		if (abs_deviation <= cfg->precision) {
			schedtab->state = SCHEDTAB_STATE_RUNNING_SYNC;
		} else {
			schedtab->state = SCHEDTAB_STATE_RUNNING_ASYNC;
		}
	}

	/* Wait + re-arm alarm. We misuse alarm->cycle to set the next expiry */
	assert(last_action == SCHEDTAB_ACTION_WAIT);
	assert(alm->state == ALARM_STATE_IDLE);
	alm->cycle = wait_time;
	return;

stop_this_schedtab:
	schedtab->state = SCHEDTAB_STATE_STOPPED;
	alm->cycle = 0;
	return;
}

void sys_schedtab_start_rel(
	unsigned int schedtab_id,
	ctrtick_t offset)
{
	const struct counter_cfg *ctr_cfg;
	const struct part_cfg *part_cfg;
	const struct schedtab_cfg *cfg;
	struct schedtab *schedtab;
	struct alarm *alm;
	ctrtick_t first_expiry;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);
	if (schedtab_id >= part_cfg->num_schedtabs) {
		SET_RET(E_OS_ID);
		return;
	}

	/* validate schedule table type and state */
	schedtab = &part_cfg->schedtabs[schedtab_id];
	cfg = &schedtab_cfg[schedtab->schedtab_id];

	/* SYS_Os_00452: bail out on OsScheduleTblSyncStrategy == IMPLICIT */
	if (cfg->flags & SCHEDTAB_FLAG_SYNC_IMPLICIT) {
		SET_RET(E_OS_ID);
		return;
	}
	if (schedtab->state != SCHEDTAB_STATE_STOPPED) {
		SET_RET(E_OS_STATE);
		return;
	}

	/* validate relative offset */
	// FIXME: SWS_Os_00276 says: ... minus the Initial Offset !?!?!?
	// FIXME: we need to figure out the delay of the first expiry point!
	first_expiry = 0;
	ctr_cfg = &counter_cfg[cfg->counter_id];
	assert(first_expiry <= ctr_cfg->maxallowedvalue);
	if ((offset == 0) || (offset > ctr_cfg->maxallowedvalue - first_expiry)) {
		SET_RET(E_OS_VALUE);
		return;
	}

	SET_RET(E_OK);

	/* start schedule table */
	schedtab_start(schedtab, cfg);

	/* arm relative alarm */
	alm = cfg->alarm;
	assert(alm->state == ALARM_STATE_IDLE);
	alarm_set_rel(alm, ctr_cfg, offset, 0);
}

void sys_schedtab_start_abs(
	unsigned int schedtab_id,
	ctrtick_t start)
{
	const struct counter_cfg *ctr_cfg;
	const struct part_cfg *part_cfg;
	const struct schedtab_cfg *cfg;
	struct schedtab *schedtab;
	struct alarm *alm;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);
	if (schedtab_id >= part_cfg->num_schedtabs) {
		SET_RET(E_OS_ID);
		return;
	}

	/* validate schedule table type and state */
	schedtab = &part_cfg->schedtabs[schedtab_id];
	cfg = &schedtab_cfg[schedtab->schedtab_id];

	if (schedtab->state != SCHEDTAB_STATE_STOPPED) {
		SET_RET(E_OS_STATE);
		return;
	}

	/* validate relative offset */
	ctr_cfg = &counter_cfg[cfg->counter_id];
	if (start > ctr_cfg->maxallowedvalue) {
		SET_RET(E_OS_VALUE);
		return;
	}

	SET_RET(E_OK);

	/* start schedule table */
	schedtab_start(schedtab, cfg);

	/* arm absolute alarm */
	alm = cfg->alarm;
	assert(alm->state == ALARM_STATE_IDLE);
	alarm_set_abs(alm, ctr_cfg, start, 0);
}

void sys_schedtab_stop(
	unsigned int schedtab_id)
{
	const struct part_cfg *part_cfg;
	struct schedtab *schedtab;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);
	if (schedtab_id >= part_cfg->num_schedtabs) {
		SET_RET(E_OS_ID);
		return;
	}

	schedtab = &part_cfg->schedtabs[schedtab_id];

	if (schedtab->state == SCHEDTAB_STATE_STOPPED) {
		SET_RET(E_OS_NOFUNC);
		return;
	}

	SET_RET(E_OK);

	schedtab_stop(schedtab);
}

void schedtab_stop(struct schedtab *schedtab)
{
	const struct schedtab_cfg *cfg;
	struct schedtab *schedtab_old_next;
	struct alarm *alm;

	assert(schedtab != NULL);
	assert(schedtab->state != SCHEDTAB_STATE_STOPPED);
	cfg = &schedtab_cfg[schedtab->schedtab_id];


	if (schedtab->state == SCHEDTAB_STATE_NEXT) {
		/* we cannot really de-register a schedule table due to the lack of
		 * a link back to the source schedule table.
		 * Just set it to STOPPED, and it will be stopped on the table switch.
		 */
		schedtab->state = SCHEDTAB_STATE_STOPPED;
	} else if (schedtab->state == SCHEDTAB_STATE_WAITING) {
		/* WAITING state has no active alarm */
		schedtab->state = SCHEDTAB_STATE_STOPPED;
	} else {
		assert(schedtab->state == SCHEDTAB_STATE_RUNNING ||
		       schedtab->state == SCHEDTAB_STATE_RUNNING_SYNC ||
		       schedtab->state == SCHEDTAB_STATE_RUNNING_ASYNC);

		/* de-register "next" to this table, if any */
		schedtab_old_next = schedtab->next;
		if (schedtab_old_next != NULL) {
			/* for repeating tables, a schedule table may have itself as next */
			if (schedtab != schedtab_old_next) {
				/* the target schedule table can be in stopped state already */
				assert((schedtab_old_next->state == SCHEDTAB_STATE_STOPPED) ||
				       (schedtab_old_next->state == SCHEDTAB_STATE_NEXT));
				schedtab_old_next->state = SCHEDTAB_STATE_STOPPED;
			}
		}

		/* disarm alarm */
		alm = cfg->alarm;
		assert(alm->state == ALARM_STATE_ACTIVE);
		alarm_cancel(alm);
		schedtab->state = SCHEDTAB_STATE_STOPPED;
	}
}

void sys_schedtab_next(
	unsigned int schedtab_id_from,
	unsigned int schedtab_id_to)
{
	const struct schedtab_cfg *cfg_from;
	const struct schedtab_cfg *cfg_to;
	const struct part_cfg *part_cfg;
	struct schedtab *schedtab_from;
	struct schedtab *schedtab_to;
	struct schedtab *schedtab_old_next;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);

	if ((schedtab_id_from >= part_cfg->num_schedtabs) ||
	    (schedtab_id_to >= part_cfg->num_schedtabs)) {
		SET_RET(E_OS_ID);
		return;
	}

	schedtab_from = &part_cfg->schedtabs[schedtab_id_from];
	schedtab_to = &part_cfg->schedtabs[schedtab_id_to];

	cfg_from = &schedtab_cfg[schedtab_from->schedtab_id];
	cfg_to = &schedtab_cfg[schedtab_to->schedtab_id];

	/* both schedule tables must be driven by same counter */
	if (cfg_from->counter_id != cfg_to->counter_id) {
		SET_RET(E_OS_ID);
		return;
	}

	/* ... and have the same synchronization strategy */
	if ((cfg_from->flags & SCHEDTAB_SYNC_FLAGS) !=
	    (cfg_to->flags & SCHEDTAB_SYNC_FLAGS)) {
		SET_RET(E_OS_ID);
		return;
	}

	if ((schedtab_from->state == SCHEDTAB_STATE_STOPPED) ||
	    (schedtab_from->state == SCHEDTAB_STATE_NEXT)) {
		SET_RET(E_OS_NOFUNC);
		return;
	}

	if (schedtab_to->state != SCHEDTAB_STATE_STOPPED) {
		SET_RET(E_OS_STATE);
		return;
	}

	SET_RET(E_OK);

	/* de-register old "next", if any */
	schedtab_old_next = schedtab_from->next;
	if (schedtab_old_next != NULL) {
		/* for repeating tables, a schedule table may have itself as next */
		if (schedtab_from != schedtab_old_next) {
			/* the target schedule table can be in stopped state already */
			assert((schedtab_old_next->state == SCHEDTAB_STATE_STOPPED) ||
			       (schedtab_old_next->state == SCHEDTAB_STATE_NEXT));
			schedtab_old_next->state = SCHEDTAB_STATE_STOPPED;
		}
	}

	/* register "to" as "next" in "from" */
	schedtab_from->next = schedtab_to;

	schedtab_to->state = SCHEDTAB_STATE_NEXT;
}

void sys_schedtab_start_sync(
	unsigned int schedtab_id)
{
	const struct part_cfg *part_cfg;
	const struct schedtab_cfg *cfg;
	struct schedtab *schedtab;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);
	if (schedtab_id >= part_cfg->num_schedtabs) {
		SET_RET(E_OS_ID);
		return;
	}

	/* validate schedule table type and state */
	schedtab = &part_cfg->schedtabs[schedtab_id];
	cfg = &schedtab_cfg[schedtab->schedtab_id];

	if (!(cfg->flags & SCHEDTAB_FLAG_SYNC_EXPLICIT)) {
		SET_RET(E_OS_ID);
		return;
	}

	if (schedtab->state != SCHEDTAB_STATE_STOPPED) {
		SET_RET(E_OS_STATE);
		return;
	}

	SET_RET(E_OK);

	schedtab->state = SCHEDTAB_STATE_WAITING;
}

void sys_schedtab_sync(
	unsigned int schedtab_id,
	ctrtick_t value)
{
	const struct counter_cfg *ctr_cfg;
	const struct part_cfg *part_cfg;
	const struct schedtab_cfg *cfg;
	struct schedtab *schedtab;
	struct counter *ctr;
	struct alarm *alm;
	ctrtick_t current;
	ctrtick_t elapsed;
	ctrtick_t abs_deviation;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);
	if (schedtab_id >= part_cfg->num_schedtabs) {
		SET_RET(E_OS_ID);
		return;
	}

	/* validate schedule table type and state */
	schedtab = &part_cfg->schedtabs[schedtab_id];
	cfg = &schedtab_cfg[schedtab->schedtab_id];

	if (!(cfg->flags & SCHEDTAB_FLAG_SYNC_EXPLICIT)) {
		SET_RET(E_OS_ID);
		return;
	}

	if (value >= cfg->duration) {
		SET_RET(E_OS_VALUE);
		return;
	}

	if ((schedtab->state == SCHEDTAB_STATE_STOPPED) ||
	    (schedtab->state == SCHEDTAB_STATE_NEXT)) {
		SET_RET(E_OS_STATE);
		return;
	}

	SET_RET(E_OK);

	/* initially start schedule table */
	if (schedtab->state == SCHEDTAB_STATE_WAITING) {
		schedtab_start(schedtab, cfg);
		schedtab->state = SCHEDTAB_STATE_RUNNING_SYNC;

		/* arm absolute alarm */
		alm = cfg->alarm;
		assert(alm->state == ALARM_STATE_IDLE);
		ctr_cfg = &counter_cfg[cfg->counter_id];
		alarm_set_rel(alm, ctr_cfg, cfg->duration - value, 0);

		assert(cfg->duration <= ctr_cfg->maxallowedvalue);

		/* find initial offset between sync counter and drive counter */
		ctr = ctr_cfg->counter;
		schedtab->sync_counter_offset = ctr_diff(value, ctr->current,
		                                         ctr_cfg->maxallowedvalue);
		schedtab->deviation = 0;
	} else {
		/* already running, try synching */

		/* get current counter value */
		ctr_cfg = &counter_cfg[cfg->counter_id];
		current = counter_query(ctr_cfg);

		/* current position in the schedule table */
		elapsed = ctr_diff(schedtab->sync_counter_offset, current,
		                   ctr_cfg->maxallowedvalue);
		/* elapsed must be in range as sync_counter_offset is updated
		 * during the schedule table's START expiry point */
		assert(elapsed < cfg->duration);

		abs_deviation = ctr_diff(value, elapsed, cfg->duration - 1);

		if (abs_deviation <= cfg->duration / 2) {
			/* schedule table runs too fast */
			schedtab->deviation = abs_deviation;
		} else {
			/* schedule table runs too slow */
			abs_deviation = cfg->duration - abs_deviation;
			schedtab->deviation = -abs_deviation;
		}

		if (abs_deviation <= cfg->precision) {
			schedtab->state = SCHEDTAB_STATE_RUNNING_SYNC;
		} else {
			schedtab->state = SCHEDTAB_STATE_RUNNING_ASYNC;
		}

		/* deviation adjustment happens on the next expiry point */
	}
}

void sys_schedtab_set_async(
	unsigned int schedtab_id)
{
	const struct part_cfg *part_cfg;
	const struct schedtab_cfg *cfg;
	struct schedtab *schedtab;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);
	if (schedtab_id >= part_cfg->num_schedtabs) {
		SET_RET(E_OS_ID);
		return;
	}

	/* validate schedule table type and state */
	schedtab = &part_cfg->schedtabs[schedtab_id];
	cfg = &schedtab_cfg[schedtab->schedtab_id];

	if (!(cfg->flags & SCHEDTAB_FLAG_SYNC_EXPLICIT)) {
		SET_RET(E_OS_ID);
		return;
	}

	if ((schedtab->state == SCHEDTAB_STATE_STOPPED) ||
	    (schedtab->state == SCHEDTAB_STATE_NEXT) ||
	    (schedtab->state == SCHEDTAB_STATE_WAITING)) {
		SET_RET(E_OS_STATE);
		return;
	}

	SET_RET(E_OK);

	assert((schedtab->state == SCHEDTAB_STATE_RUNNING) ||
	       (schedtab->state == SCHEDTAB_STATE_RUNNING_SYNC) ||
	       (schedtab->state == SCHEDTAB_STATE_RUNNING_ASYNC));

	schedtab->state = SCHEDTAB_STATE_RUNNING;
}

void sys_schedtab_get_state(
	unsigned int schedtab_id)
{
	const struct part_cfg *part_cfg;
	struct schedtab *schedtab;

	part_cfg = current_part_cfg();
	assert(part_cfg != NULL);
	if (schedtab_id >= part_cfg->num_schedtabs) {
		SET_RET(E_OS_ID);
		return;
	}

	schedtab = &part_cfg->schedtabs[schedtab_id];

	/* state translation done in user space (if required) */
	SET_OUT1(schedtab->state);
	SET_RET(E_OK);
}

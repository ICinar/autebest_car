/*
 * schedtab.h
 *
 * Scheduling tables (schedtab)
 *
 * azuepke, 2014-06-09: initial
 */

#ifndef __SCHEDTAB_H__
#define __SCHEDTAB_H__

#include <schedtab_state.h>
#include <alarm_state.h>

/** Scheduling table configuration -> config.c */
extern const uint8_t num_schedtabs;
extern const struct schedtab_cfg schedtab_cfg[];

/** Scheduling table actions -> config.c */
extern struct schedtab_action_cfg schedtab_action_cfg[];

void schedtab_init_all(void);
void schedtab_expire(struct alarm *alm, struct schedtab *schedtab);
void schedtab_stop(struct schedtab *schedtab);


/* system calls */
__tc_fastcall void sys_schedtab_start_rel(
	unsigned int schedtab_id,
	ctrtick_t offset);

__tc_fastcall void sys_schedtab_start_abs(
	unsigned int schedtab_id,
	ctrtick_t start);

__tc_fastcall void sys_schedtab_stop(
	unsigned int schedtab_id);

__tc_fastcall void sys_schedtab_next(
	unsigned int schedtab_id_from,
	unsigned int schedtab_id_to);

__tc_fastcall void sys_schedtab_start_sync(
	unsigned int schedtab_id);

__tc_fastcall void sys_schedtab_sync(
	unsigned int schedtab_id,
	ctrtick_t value);

__tc_fastcall void sys_schedtab_set_async(
	unsigned int schedtab_id);

__tc_fastcall void sys_schedtab_get_state(
	unsigned int schedtab_id);

#endif

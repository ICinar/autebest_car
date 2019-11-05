/*
 * alarm.h
 *
 * OSEK alarms
 *
 * azuepke, 2014-06-05: initial
 */

#ifndef __ALARM_H__
#define __ALARM_H__

#include <alarm_state.h>
#include <hv_types.h>

/** Alarm configuration -> config.c */
extern const struct alarm_cfg alarm_cfg[];

void alarm_init_all(void);
void alarm_enqueue(struct alarm *alm, const struct counter_cfg *ctr_cfg);
void alarm_enqueue_first(struct alarm *alm);
void alarm_expire(struct alarm *alm);
void alarm_cancel(struct alarm *alm);
void alarm_set_rel(struct alarm *alm, const struct counter_cfg *ctr_cfg, ctrtick_t increment, ctrtick_t cycle);
void alarm_set_abs(struct alarm *alm, const struct counter_cfg *ctr_cfg, ctrtick_t expiry, ctrtick_t cycle);


/** Get static configuration of the alarm */
__tc_fastcall void sys_alarm_base(unsigned int alarm_id);
/** Get relative number of ticks before the alarm expires */
__tc_fastcall void sys_alarm_get(unsigned int alarm_id);
/** Set relative number of ticks before the alarm expires */
__tc_fastcall void sys_alarm_set_rel(unsigned int alarm_id, ctrtick_t increment, ctrtick_t cycle);
/** Set relative number of ticks before the alarm expires */
__tc_fastcall void sys_alarm_set_abs(unsigned int alarm_id, ctrtick_t expiry, ctrtick_t cycle);
/** Cancel an alarm (before it expires) */
__tc_fastcall void sys_alarm_cancel(unsigned int alarm_id);

#endif

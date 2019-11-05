/*
 * system_timer.h
 *
 * OSEK counter-based system timer
 *
 * azuepke, 2014-09-08: stubbed out from board layer
 */

#ifndef __SYSTEM_TIMER_H__
#define __SYSTEM_TIMER_H__

#include <stdint.h>
#include <hv_compiler.h>
#include <hv_types.h>

/** register system timer counter (called once at boot time by the kernel) */
void system_timer_register(const struct counter_cfg *ctr_cfg);

/** get current system time (in time units of the system timer, e.g. microseconds) */
ctrtick_t system_timer_query(const struct counter_cfg *ctr_cfg);

/** change timer expiry time (can be found in the first alarm in ctr) */
void system_timer_change(const struct counter_cfg *ctr_cfg);

/** increment the system timer counter */
void system_timer_increment(void);

#endif

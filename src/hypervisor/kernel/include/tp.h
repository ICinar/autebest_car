/*
 * tp.h
 *
 * Scheduling tables (schedtab)
 *
 * azuepke, 2015-04-22: initial
 */

#ifndef __TP_H__
#define __TP_H__

#include <tp_state.h>

/** Time partition windows -> config.c */
extern const struct tpwindow_cfg tpwindow_cfg[];

/** Time partition schedules -> config.c */
extern const uint8_t num_tpschedules;
extern const struct tpschedule_cfg tpschedule_cfg[];

#endif

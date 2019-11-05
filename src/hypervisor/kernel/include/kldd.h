/*
 * kldd.h
 *
 * Kernel level device drivers (KLDDs)
 *
 * azuepke, 2014-03-09: initial
 */

#ifndef __KLDD_H__
#define __KLDD_H__

#include <kldd_state.h>

/** KLDD configuration -> config.c */
extern const struct kldd_cfg kldd_cfg[];

/** Call a kernel level device driver (KLDD) identified by its ID kldd_id */
__tc_fastcall void sys_kldd_call(unsigned int kldd_id, unsigned long arg1,
                           unsigned long arg2, unsigned long arg3);

#endif

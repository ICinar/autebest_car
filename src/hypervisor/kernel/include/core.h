/*
 * core.h
 *
 * Kernel internal per-core state.
 *
 * azuepke, 2015-03-23: initial
 */

#ifndef __CORE_H__
#define __CORE_H__

#include <core_state.h>

extern const struct core_cfg core_cfg[];
extern const size_t kern_stack_size;
extern const size_t nmi_stack_size;
extern const size_t idle_stack_size;
extern const uint8_t kern_num_ctxts;
extern const uint8_t nmi_num_ctxts;
extern const uint8_t idle_num_ctxts;

#endif

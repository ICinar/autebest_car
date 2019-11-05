/*
 * arch_mpu.h
 *
 * Architecture specific MPU handling
 *
 * azuepke, 2015-01-06: initial from arch.h
 */

#ifndef __ARCH_MPU_H__
#define __ARCH_MPU_H__

#include <arch_mpu_state.h>

void arch_mpu_part_switch(const struct arch_mpu_part_cfg *cfg);
void arch_mpu_task_switch(const struct arch_mpu_task_cfg *cfg);

#endif

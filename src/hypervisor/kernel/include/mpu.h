/*
 * mpu.h
 *
 * Common MPU handling
 *
 * azuepke, 2015-03-23: initial (from config.h)
 */

#ifndef __MPU_H__
#define __MPU_H__

#include <arch_mpu_state.h>

/** partition specific MPU configuration (one entry for each partition) */
extern const struct arch_mpu_part_cfg mpu_part_cfg[];

/** task specific MPU configuration (one entry for each task) */
extern const struct arch_mpu_task_cfg mpu_task_cfg[];

#endif

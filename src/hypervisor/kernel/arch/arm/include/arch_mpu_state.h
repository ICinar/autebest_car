/*
 * arch_mpu_state.h
 *
 * ARM address space data type and operations.
 *
 * azuepke, 2013-11-24: initial MPU version
 * azuepke, 2014-08-05: revised MPU ersion for ARM Cortex-R4
 * azuepke, 2015-06-28: add Cortex-M3/M4
 */

#ifndef __ARCH_MPU_STATE_H__
#define __ARCH_MPU_STATE_H__

#include <stdint.h>

/** an MPU region on ARM -- the structure resembles the MPU register layout */
struct arch_mpu_region {
#ifdef ARM_CORTEXM
	uint32_t base_region_valid;
	uint32_t attrib_size_enable;
#else
	uint32_t base;
	uint32_t size_enable;
	uint32_t access_control;
#endif
};

#ifdef ARM_MPU_8
#define ARCH_MPU_REGIONS_KERN   0
#define ARCH_MPU_REGIONS_PART   7
#define ARCH_MPU_REGIONS_TASK   1
#elif defined ARM_MPU_12
#define ARCH_MPU_REGIONS_KERN   0
#define ARCH_MPU_REGIONS_PART  11
#define ARCH_MPU_REGIONS_TASK   1
#elif defined ARM_MPU_16
#define ARCH_MPU_REGIONS_KERN   0
#define ARCH_MPU_REGIONS_PART  15
#define ARCH_MPU_REGIONS_TASK   1
#else
#define ARCH_MPU_REGIONS_KERN   0
#define ARCH_MPU_REGIONS_PART   0
#define ARCH_MPU_REGIONS_TASK   0
#endif

/** partition specific MPU configuration */
struct arch_mpu_part_cfg {
#ifdef ARM_MMU
	/* pointer/address of per partition page-tables */
	/* ttbr0 MUST COME FIRST -- used by startup assembler code */
	uint32_t ttbr0;
	uint32_t asid;
#else
	struct arch_mpu_region region[ARCH_MPU_REGIONS_PART];
#endif
};

/** task specific MPU configuration */
struct arch_mpu_task_cfg {
#ifdef ARM_MMU
	/* not used */
#else
	struct arch_mpu_region region[ARCH_MPU_REGIONS_TASK];
#endif
};

#endif

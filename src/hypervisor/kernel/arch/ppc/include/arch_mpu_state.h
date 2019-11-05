/*
 * arch_mpu_state.h
 *
 * PPC address space data type and operations.
 *
 * azuepke, 2014-06-03: initial
 * azuepke, 2014-08-05: revised MPU version for PowerPC e200 cores
 */

#ifndef __ARCH_MPU_STATE_H__
#define __ARCH_MPU_STATE_H__

#include <stdint.h>

/** an MPU region on PPC -- the structure resembles the MAS registers */
struct arch_mpu_region
{
#ifdef MPC5748G
	/* Start address, goes into SMPUx_RGDn_WRD0 */
	uint32_t start_address;
	/* End address, goes into SMPUx_RGDn_WRD1 */
	uint32_t end_address;
	/* Pointers for each bus master to select access control flags
	 * contained in Word3. Goes into SMPUx_RD_nW2_F1*/
	uint32_t access_pointers;
	/* Permissions flags used when FMT = 1, goes in SMPUx_RGDn_WRD3[ACCSET1] */
	uint32_t permissions;
#else
	uint32_t mas1;
	uint32_t mas2;
	uint32_t mas3;
#endif
};

#ifdef MPC5748G
/* On the MPC5748G we use the System Memory Protection Unit (SMPU) instead of
 * the TLB entries. The SMPU has 16 region descriptors. We use at the moment
 * all of them for the partition switch.
 *
 * Implementation note:
 * The settings in the region descriptors may overlap and override each other.
 * In order to force the SMPU use only certain regions, one can invalidate the
 * other regions by setting their corresponding RGDn_WRD5[VLD] to 0.
 */
#define ARCH_MPU_REGIONS_KERN   0
#define ARCH_MPU_REGIONS_PART  16
#define ARCH_MPU_REGIONS_TASK   0
#else
/* we have 16 (e200z4) or 32 (e200z6) entries in TLB1,
 * but we restrict the tooling to 16 TLB entries for now:
 * the last 4 are reserved for the kernel,
 * leaving 12 for user space */
#define ARCH_MPU_REGIONS_KERN   4
#define ARCH_MPU_REGIONS_PART  10
#define ARCH_MPU_REGIONS_TASK   2
#endif /* ifdef MPC5748G */

/** partition specific MPU configuration */
struct arch_mpu_part_cfg {
	struct arch_mpu_region region[ARCH_MPU_REGIONS_PART];
};

/** task specific MPU configuration */
struct arch_mpu_task_cfg {
	struct arch_mpu_region region[ARCH_MPU_REGIONS_TASK];
};

#endif

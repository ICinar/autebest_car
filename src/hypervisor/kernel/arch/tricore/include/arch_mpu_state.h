/*
 * arch_mpu_state.h
 *
 * TriCore address space data type and operations.
 *
 * azuepke, 2014-10-25: initial
 * azuepke, 2014-02-20: implementation
 * azuepke, 2014-03-06: adapt to SMP
 */

#ifndef __ARCH_MPU_STATE_H__
#define __ARCH_MPU_STATE_H__

#include <stdint.h>

/** an MPU range on Tricore comprises lower and upper bounds
 *  with valid address matching:  lower <= addr < upper
 */
struct arch_mpu_range {
	uint32_t lower;
	uint32_t upper;
};

/* NOTE: this describes the architecture level TC1.6.1 MPU used in AURIX
 *
 * We have 16 data ranges on AURIX, which we use in the following way:
 * - the first 9 ranges are "per partition"
 * - we have 2 ranges for "per-task" usage
 * - the last 5 are reserved for the kernel
 *
 * Additionally, we have 8 code ranges on AURIX. We skip "per-task" usage:
 * - the first 5 ranges are "per partition"
 * - the last 3 are reserved for the kernel
 *
 * And since we have two independent set of ranges for code and data,
 * we need to program both of them. Of the four sets, we use:
 * - permission set 0 is used by the kernel
 * - permission set 1 is used by user space
 * - permission set 2 and 3 are not used
 *
 * For invalid ranges, we use both lower and upper equal zero.
 *
 * Access permissions. We assume:
 * - exec: the kernel uses all 3 of its executing windows
 * - exec: the user uses all of its 9 ranges for execution
 * - exec: the per-task windows never switch execution windows
 * - exec: kernel and user code exection are disjunctive
 * - read: assume all windows used
 * - write: the user has a special set
 * - write: the kernel's set implies the user's set
 * - write: the first window of the kernel is read-only (flash mapping)
 *
 * Thus we set all read permissions to 1 and use a hardcoded set for execution
 * permissions. Since a task's is a part of a partition, we switch write
 * permissions on task switches (which happen directly after the part-switch).
 *
 * This results in (encoded in binary, MSB first)
 * - dpre_0 (kernel read):   1111 1111 1111 1111 -> 0xffff
 * - dpre_1 (user   read):   0000 0111 1111 1111 -> 0x07ff
 * - cpxe_0 (kernel exec):             1110 0000 -> 0x00e0
 * - cpxe_1 (user   exec):             0001 1111 -> 0x001f
 * - dpwe_0 (kernel write):  kkkk 0uuu uuuu uuuu -> dpwe_1 | 0xf000
 * - dpwe_1 (user   write):  0000 0uuu uuuu uuuu -> tool generated
 */
#define ARCH_MPU_DATA_RANGES_PART   9
#define ARCH_MPU_DATA_RANGES_TASK   2
#define ARCH_MPU_DATA_RANGES_KERN   5

#define ARCH_MPU_CODE_RANGES_PART   5
#define ARCH_MPU_CODE_RANGES_KERN   3

/** partition specific MPU configuration */
struct arch_mpu_part_cfg {
	struct arch_mpu_range data[ARCH_MPU_DATA_RANGES_PART];
	struct arch_mpu_range code[ARCH_MPU_CODE_RANGES_PART];
};

/** task specific MPU configuration */
struct arch_mpu_task_cfg {
	struct arch_mpu_range data[ARCH_MPU_DATA_RANGES_TASK];
	uint32_t dpwe;	/* write permissions */
};

#endif

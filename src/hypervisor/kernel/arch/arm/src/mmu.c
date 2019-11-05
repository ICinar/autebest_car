/*
 * mmu.c
 *
 * Architecture specific MMU handling
 *
 * azuepke, 2015-08-03: initial
 */

#include <kernel.h>
#include <assert.h>
#include <hv_compiler.h>
#include <arch_mpu.h>

/* inner: WA + shareable, outer: WA + not shareable */
#define WALK_MODE	0x02b

/** prepare MPU for next partition on partition switch */
void arch_mpu_part_switch(const struct arch_mpu_part_cfg *cfg __unused)
{
	assert(cfg != NULL);

	/* update per-partition page table */
	arm_dsb();	/* ARM errata 754322 for Cortex A9 */
	arm_set_asid(cfg->asid);
	arm_isb();
	arm_set_ttbr0(cfg->ttbr0 | WALK_MODE);
	arm_isb();
}

/** prepare MPU for next task on task switch */
void arch_mpu_task_switch(const struct arch_mpu_task_cfg *cfg __unused)
{
	assert(cfg != NULL);
}

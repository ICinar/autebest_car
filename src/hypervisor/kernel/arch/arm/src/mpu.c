/*
 * mpu.c
 *
 * Architecture specific MPU handling
 *
 * azuepke, 2013-11-25: initial
 * azuepke, 2014-08-25: enable MPU (Cortex-R4)
 */

#include <kernel.h>
#include <assert.h>
#include <hv_compiler.h>
#include <arch_mpu.h>


/** prepare MPU for next partition on partition switch */
void arch_mpu_part_switch(const struct arch_mpu_part_cfg *cfg __unused)
{
	assert(cfg != NULL);

#if defined ARM_MPU_8 || defined ARM_MPU_12 || defined ARM_MPU_16
	/* update MPU windows: first disable the window, finally enable it again */
	for (unsigned int i = 0; i < ARCH_MPU_REGIONS_PART; i++) {
		arm_mpu_rgnr(ARCH_MPU_REGIONS_KERN + i);
		arm_mpu_size_enable(0);
		arm_mpu_base(cfg->region[i].base);
		arm_mpu_access_control(cfg->region[i].access_control);
		arm_mpu_size_enable(cfg->region[i].size_enable);
	}
#endif
}

/** prepare MPU for next task on task switch */
void arch_mpu_task_switch(const struct arch_mpu_task_cfg *cfg __unused)
{
	assert(cfg != NULL);

#if defined ARM_MPU_8 || defined ARM_MPU_12 || defined ARM_MPU_16
	/* update MPU windows: first disable the window, finally enable it again */
	for (unsigned int i = 0; i < ARCH_MPU_REGIONS_TASK; i++) {
		arm_mpu_rgnr(ARCH_MPU_REGIONS_KERN + ARCH_MPU_REGIONS_PART + i);
		arm_mpu_size_enable(0);
		arm_mpu_base(cfg->region[i].base);
		arm_mpu_access_control(cfg->region[i].access_control);
		arm_mpu_size_enable(cfg->region[i].size_enable);
	}
#endif
}

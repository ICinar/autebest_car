/*
 * mpu.c
 *
 * Architecture specific MPU handling
 *
 * azuepke, 2015-06-30: initial
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
		MPU_RNR = ARCH_MPU_REGIONS_KERN + i;
		MPU_RASR = 0;
		barrier();
		MPU_RBAR = cfg->region[i].base_region_valid;
		MPU_RASR = cfg->region[i].attrib_size_enable;
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
		MPU_RNR = ARCH_MPU_REGIONS_KERN + ARCH_MPU_REGIONS_PART + i;
		MPU_RASR = 0;
		barrier();
		MPU_RBAR = cfg->region[i].base_region_valid;
		MPU_RASR = cfg->region[i].attrib_size_enable;
	}
#endif
}

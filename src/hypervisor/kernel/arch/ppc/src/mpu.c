/**
 * \file     mpu.c
 * \brief    Architecture specific MPU handling
 * \details  
 *
 * \author   Alex Zuepke <alexander.zuepke@hs-rm.de>
 * \author   Liviu Beraru <Liviu.Beraru@easycore.com>
 * \author   easycore GmbH, 91058 Erlangen, Germany
 *
 * \par          License
 * Customer:     @@LicenseCustomer@@,
 * License type: @@LicenseType@@,
 * Licensed for project: @@LicenseProject@@.
 *
 * \par      File history
 * - azuepke, 2014-06-03: initial
 * - azuepke, 2014-08-25: enable MPU (using the Book-E MMU)
 * - lberaru, 2015-10-20: multicore Calypso
 * 
 * \copyright Copyright 2014-2015 easycore GmbH, 91058 Erlangen, Germany.
 * All rights exclusively reserved for easycore GmbH, unless expressly agreed
 * to otherwise.
 */

/*==================[inclusions]==============================================*/

#include <kernel.h>
#include <assert.h>
#include <hv_compiler.h>
#include <arch_mpu.h>
#include <ppc_tlb.h>

#ifdef MPC5748G
#include "smpu.h"
#endif

/*==================[macros]==================================================*/
/*==================[type definitions]========================================*/
/*==================[external function declarations]==========================*/
/*==================[internal function declarations]==========================*/

#if (defined SMP) && (defined MPC5748G) /* Calypso multicore */
static inline void override_partition_regions
(
    const uint32_t start_region,
    const uint32_t end_region,
    const struct arch_mpu_part_cfg *cfg
);
#endif

/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/
/*==================[external function definitions]===========================*/

/*------------------[Partition switch]----------------------------------------*/

/** 
 * \brief Prepare MPU for next partition on partition switch.
 */

#if (defined MPC5748G) /* Calypso */
/*==================[Calypso partition switch]================================*/
void arch_mpu_part_switch(const struct arch_mpu_part_cfg *cfg __unused)
{
#if (defined USE_MPU) /* if the MPU should be used */
    
    #if (defined SMP) /* Multicore */

    assert(cfg != NULL);

    /* This partly obscure code implements the usage of the 16 SMPU regions
     * documented in the picture Calypso_Multicore_SMPU.png.
     * See also the documentation in bsp/mpc5748g/include/smpu.h.
     */
    switch (arch_cpu_id())
    {
        case 0:
            override_partition_regions(2, 5, cfg);
            break;

        case 1:
            override_partition_regions(7, 10, cfg);
            break;

        case 2:
            override_partition_regions(12, 15, cfg);
            break;

        default:
            break;
    }

    #else /* Single core */


    unsigned int start_address = 0;
    unsigned int smpu_region   = 2;
    unsigned int cfg_region    = 0;

    assert(cfg != NULL);

    /* Disable SMPU before setting the new region descriptors */
    SMPU_0_CES0 = SMPU_CES0_DISABLE;
    SMPU_1_CES0 = SMPU_CES0_DISABLE;

    while (smpu_region < ARCH_MPU_REGIONS_PART)
    {
        start_address = cfg->region[cfg_region].start_address;

        if (IS_RAM_ADDRESS(start_address))
        {
            smpu_write_region(SMPU1, smpu_region, &(cfg->region[cfg_region]));
            smpu_invalidate_region(SMPU0, smpu_region);
        }
        else if (start_address == 0x00u)
        {
            smpu_invalidate_region(SMPU0, smpu_region);
            smpu_invalidate_region(SMPU1, smpu_region);
        }
        else
        {
            smpu_write_region(SMPU0, smpu_region, &(cfg->region[cfg_region]));
            smpu_invalidate_region(SMPU1, smpu_region);
        }

        smpu_region++;
        cfg_region++;
    }

    /* Reenable SMPU */
    SMPU_0_CES0 = SMPU_CES0_ENABLE;
    SMPU_1_CES0 = SMPU_CES0_ENABLE;

    #endif /* if defined SMP */
#endif /* if defined USE_MPU */
}


#else
/*==================[Bolero partition switch]=================================*/
void arch_mpu_part_switch(const struct arch_mpu_part_cfg *cfg)
{
    unsigned long mas0, mas1, mas2, mas3;
    unsigned int mpu_regions_part = ARCH_MPU_REGIONS_PART;
    unsigned int i;

    assert(cfg != NULL);

    /* update MMU entries 0..9 */
    for (i = 0; i < mpu_regions_part; i++)
    {
        mas0 = MAS0_TLBSEL(1) | MAS0_ESEL(0 + i);
        mas1 = cfg->region[i].mas1;
        mas2 = cfg->region[i].mas2;
        mas3 = cfg->region[i].mas3;
        ppc_set_spr(SPR_MAS0, mas0);
        ppc_set_spr(SPR_MAS1, mas1);
        ppc_set_spr(SPR_MAS2, mas2);
        ppc_set_spr(SPR_MAS3, mas3);
        ppc_isync();
        ppc_tlbwe();
        ppc_isync();
    }
}
#endif

/*------------------[Task switch]---------------------------------------------*/

/**
 * Prepare MPU for next task on task switch.
 */
void arch_mpu_task_switch(const struct arch_mpu_task_cfg *cfg __unused)
{
#ifndef MPC5748G
    unsigned long mas0, mas1, mas2, mas3;
    unsigned int mpu_regions_task = ARCH_MPU_REGIONS_TASK;
    unsigned int i;

    assert(cfg != NULL);

    /* update MMU entries 10 and 11 */
    for (i = 0; i < mpu_regions_task; i++)
    {
        mas0 = MAS0_TLBSEL(1) | MAS0_ESEL(10 + i);
        mas1 = cfg->region[i].mas1;
        mas2 = cfg->region[i].mas2;
        mas3 = cfg->region[i].mas3;
        ppc_set_spr(SPR_MAS0, mas0);
        ppc_set_spr(SPR_MAS1, mas1);
        ppc_set_spr(SPR_MAS2, mas2);
        ppc_set_spr(SPR_MAS3, mas3);
        ppc_isync();
        ppc_tlbwe();
        ppc_isync();
    }

#endif
}

/*==================[internal function definitions]===========================*/

#if (defined USE_MPU) && (defined SMP) && (defined MPC5748G)

static inline void override_partition_regions
(
    const uint32_t start_region,
    const uint32_t end_region,
    const struct arch_mpu_part_cfg *cfg
)
{
    uint32_t ram_region = 0;
    uint32_t flash_region = 0;
    uint32_t start_address;
    uint32_t i = 0;

    for (i = start_region; i <= end_region; i++)
    {
        smpu_invalidate_region(SMPU0, i);
        smpu_invalidate_region(SMPU1, i);
    }

    ram_region = start_region;
    flash_region = start_region;

    for (i = 0; i < ARCH_MPU_REGIONS_PART; i++)
    {
        start_address = cfg->region[i].start_address;
        if (IS_RAM_ADDRESS(start_address))
        {
            if (ram_region <= end_region)
            {
                smpu_write_region(SMPU1, ram_region, &(cfg->region[i]));
                ram_region++;
            }
        }
        else
        {
            if (flash_region <= end_region)
            {
                smpu_write_region(SMPU0, flash_region, &(cfg->region[i]));
                flash_region++;
            }
        }
    }

}
#endif /* if (defined SMP) && (defined MPC5748G) */

/*==================[end of file]=============================================*/


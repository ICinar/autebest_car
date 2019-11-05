/**
 * \file      smpu.c
 * \brief     Implementation of the SMPU driver for MPC5748G Calypso.
 * \details   For details see smpu.h.
 *
 * \see       smpu.h
 * \see       ppc_smpu.h
 *
 * \date      20.08.2015
 * \author    Liviu Beraru <Liviu.Beraru@easycore.com>
 * \author    easycore GmbH, 91058 Erlangen, Germany
 * \version
 * \par       License
 * Customer:     @@LicenseCustomer@@,
 * License type: @@LicenseType@@,
 * Licensed for project: @@LicenseProject@@.
 *
 *
 * \copyright Copyright 2015 easycore GmbH, 91058 Erlangen, Germany.
 * All rights exclusively reserved for easycore GmbH, unless expressly agreed
 * to otherwise.
 */

/*==================[inclusions]==============================================*/

#include <kernel.h> /* arch_cpu_id */
#include <board.h>  /* declaration of board_mpu_init */
#include "smpu.h"
#include <stdint.h> /* uint32_t */

/*==================[macros]==================================================*/
/*==================[type definitions]========================================*/
/*==================[external function declarations]==========================*/
/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/
/*==================[external function definitions]===========================*/

/*------------------[Initialize SMPU]-----------------------------------------*/


/**
 * \brief Setup initial SMPU settings.
 */
void board_mpu_init(void)
{
#if (defined USE_MPU) /* if MPU usage is enabled */

    struct arch_mpu_region region;
    unsigned int accset1 = 0;

  #if (defined SMP) /* Multicore */
    /* Multicore setup */

    if (arch_cpu_id() == 0)
    {
        /* Make sure the SMPU is disabled */
        SMPU_0_CES0 = SMPU_CES0_DISABLE;
        SMPU_1_CES0 = SMPU_CES0_DISABLE;

        /*----------- RAM protecttion ----------------------------------------*/

        /* The supervisor has read access for entire RAM on all CPUs.
         * This is necessary when reading global data, mostly configuration data
         * and internal data structures.
         * Writable data is located in memory segments allocated to each CPU and
         * defined in the linker file.
         */

        accset1 = ACCSET_S_READ;

        region.start_address  = BOARD_RAM_BEGIN;
        region.end_address    = BOARD_RAM_END;
        region.access_pointers = SMPU_ALL_POINT_ACCSET1;
        region.permissions     = SMPU_WRD3_MAKE_ACCSET1(accset1) | SMPU_WRD3_FMT1;
        smpu_write_region(SMPU1, 0, &region);

        /* Write access for for every CPU in its own RAM segment. Reading is
         * done through region 0. */

        accset1 = (ACCSET_S_WRITE);

        region.start_address   = RAM_BEGIN_CPU0;
        region.end_address     = RAM_END_CPU0;
        region.access_pointers = SMPU_CPU0_POINTS_ACCSET1;
        region.permissions     = SMPU_WRD3_MAKE_ACCSET1(accset1) | SMPU_WRD3_FMT1;
        smpu_write_region(SMPU1, 1, &region);

        region.start_address   = RAM_BEGIN_CPU1;
        region.end_address     = RAM_END_CPU1;
        region.access_pointers = SMPU_CPU1_POINTS_ACCSET1;
        smpu_write_region(SMPU1, 6, &region);

        region.start_address   = RAM_BEGIN_CPU2;
        region.end_address     = RAM_END_CPU2;
        region.access_pointers = SMPU_CPU2_POINTS_ACCSET1;
        smpu_write_region(SMPU1, 11, &region);

        /*----------- Flash protecttion --------------------------------------*/

        accset1 = (ACCSET_S_READ | ACCSET_S_WRITE | ACCSET_S_EXEC);

        region.start_address   = BOARD_NVM_BEGIN;
        region.end_address     = BOARD_NVM_END;
        region.access_pointers = SMPU_ALL_POINT_ACCSET1;
        region.permissions     = SMPU_WRD3_MAKE_ACCSET1(accset1) | SMPU_WRD3_FMT1;
        smpu_write_region(SMPU0, 0, &region);

        /*----------- Peripherals protection --------------------------------*/

        /* For some reason we need exec rights for the supervisor here.
         * Which is why we leave the access rights unchanged.
         * Otherwise the peripheral SMPU will indicate in CES0[MERR] a Master 0
         * error as soon as we enable it.
         */

        region.start_address   = BOARD_PERIPHERAL_BEGIN;
        region.end_address     = BOARD_PERIPHERAL_END;
        smpu_write_region(SMPU0, 1, &region);

        /* The SMPU is enabled when all initialization has completed in function
         * board_startup_complete */
    }

  #else /* Single core */

    /* Singlecore setup */

    if (arch_cpu_id() == 0)
    {
        /* make sure SMPU is disabled before we set it up */
        SMPU_0_CES0 = SMPU_CES0_DISABLE;
        SMPU_1_CES0 = SMPU_CES0_DISABLE;

        /*----------- Flash protecttion --------------------------------------*/

        accset1 = (ACCSET_S_READ | ACCSET_S_WRITE| ACCSET_S_EXEC);

        region.start_address   = BOARD_NVM_BEGIN;
        region.end_address     = BOARD_NVM_END;
        region.access_pointers = SMPU_ALL_POINT_ACCSET1;
        region.permissions     = SMPU_WRD3_MAKE_ACCSET1(accset1) | SMPU_WRD3_FMT1;
        smpu_write_region(SMPU0, 0, &region);

        /*----------- Peripherals protection --------------------------------*/

        region.start_address   = BOARD_PERIPHERAL_BEGIN;
        region.end_address     = BOARD_PERIPHERAL_END;
        smpu_write_region(SMPU0, 1, &region);

        /*----------- RAM protecttion ----------------------------------------*/

        region.start_address   = BOARD_RAM_BEGIN;
        region.end_address     = BOARD_RAM_END;
        smpu_write_region(SMPU1, 0, &region);

        /* enable SMPU */
        SMPU_0_CES0 = SMPU_CES0_ENABLE;
        SMPU_1_CES0 = SMPU_CES0_ENABLE;
    }
  #endif /* if (defined SMP) */

#else /* Use NO SMPU */

    SMPU_0_CES0 = SMPU_CES0_DISABLE;
    SMPU_1_CES0 = SMPU_CES0_DISABLE;
#endif /* if (defined USE_MPU) */
}

/*------------------[Invalidate an MPU region]--------------------------------*/

#if (defined USE_MPU)
void smpu_invalidate_region(uint32_t smpu, uint32_t region)
{
    if ((smpu == SMPU0) || (smpu == SMPU1))
    {
        SMPUx_RGDn_WRD0(smpu, region) = 0x00u; /* zero start address*/
        SMPUx_RGDn_WRD1(smpu, region) = 0x00u; /* zero end address */
        SMPUx_RGDn_WRD2(smpu, region) = 0x00u; /* no access rights */
        SMPUx_RGDn_WRD3(smpu, region) = 0x00u; /* delete old access rights */
        SMPUx_RGDn_WRD5(smpu, region) = 0x00u; /* invalidate */
    }
}
#endif

/*------------------[Write an MPU region]-------------------------------------*/

/**
 * \brief   Write region descriptor.
 * \details This function writes the contents of the region descriptor \a
 * region_desc to the region number \a region of the SMPU unit with base address
 * \a smpu.
 *
 * \pre  The corresponding SMPU unit must be disabled when calling this function.
 * \post The regions descriptor has been written, the region has been marked
 *       as valid in the hardware and the SMPU unit is still disabled.
 *
 * \param[in] smpu   Base address of the SMPU unit.
 *                   It must be one of SMPU0 or SMPU1.
 * \param[in] region Region number where to write the region descriptor. The
 *                   SMPUs of the Calypso MPC5748G have 16 regions. First region
 *                   has number 0, last one has number 15.
 * \param[in] region_desc Contents of the region descriptor. The contents of
 *                   this descriptor is written into the first 4 words of the
 *                   hardware region descriptor.
 *
 * \warning This function does not check the contents of the region descriptor
 * \a region_desc so the caller must have previously computed the correct values.
 */
#if (defined USE_MPU)
void smpu_write_region
(
    uint32_t smpu,
    uint32_t region,
    const struct arch_mpu_region * region_desc
)
{
    if ((smpu == SMPU0) || (smpu == SMPU1))
    {
        /* set start address*/
        SMPUx_RGDn_WRD0(smpu, region) = region_desc->start_address;
        /* set end address*/
        SMPUx_RGDn_WRD1(smpu, region) = region_desc->end_address;
        /* set permission pointers in word 2 */
        SMPUx_RGDn_WRD2(smpu, region) = region_desc->access_pointers;
        /* set permissions in word 3 */
        SMPUx_RGDn_WRD3(smpu, region) = region_desc->permissions;
        /* set region valid, otherwise the SMPU will just ignore this region */
        SMPUx_RGDn_WRD5(smpu, region) = 0x01;
    }
}
#endif

/*==================[internal function definitions]===========================*/
/*==================[end of file]=============================================*/

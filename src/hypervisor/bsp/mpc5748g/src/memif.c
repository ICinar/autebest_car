/**
 * \file     memif.c
 * \brief    Initialisation of memory interface modules PRAMC and PFLASH.
 * \details
 *
 * \see      memif.h
 *
 * \date     03.12.2915
 * \author   Liviu Beraru <Liviu.Beraru@easycore.com>
 * \author   easycore GmbH, 91058 Erlangen, Germany
 *
 * \par          License
 * Customer:     @@LicenseCustomer@@,
 * License type: @@LicenseType@@,
 * Licensed for project: @@LicenseProject@@.
 *
 * \copyright Copyright 2015 easycore GmbH, 91058 Erlangen, Germany.
 * All rights exclusively reserved for easycore GmbH, unless expressly agreed
 * to otherwise.
 */

/*==================[inclusions]==============================================*/

#include "memif.h"
#include "board_stuff.h"

/*==================[macros]==================================================*/

/*------------------[Platform flash controller]-------------------------------*/

/* Platform Flash Configuration Register 1 */
#define PFLASH_PFCR1              (0xFC030000u)

/* Platform Flash Configuration Register 2 */
#define PFLASH_PFCR2              (0xFC030004u)

/* Platform Flash Configuration Register 3 */
#define PFLASH_PFCR3              (0xFC030008u)

/* Platform Flash Access Protection Register */
#define PFLASH_PFAPR              (0xFC03000Cu)

/* Platform Flash Configuration Register 4 */
#define PFLASH_PFCR4              (0xFC030018u)

/*------------------[Platform RAM Controller]---------------------------------*/

/* Platform RAM Configuration Register 1 */
#define PRAMC_0_PRCR1             (0xFC020000u)
#define PRAMC_1_PRCR1             (0xFC024000u)
#define PRAMC_2_PRCR1             (0xFC02C000u)

#define PFLASH_PFCR1_VALUE_160MHz (0x00152417u)
#define PFLASH_PFCR2_VALUE_BASIC  (0x00150017u)
#define PFLASH_PFCR3_VALUE_BASIC  (0x00000000u)
#define PFLASH_PFAPR_VALUE_BASIC  (0xFFFFFFFFu)
#define PFLASH_PFCR4_VALUE_BASIC  (0x00150017u)

/*==================[type definitions]========================================*/

/* Pointer to a function taking 2 arguments and returning nothing.
 * The array flash_write_machine_code should be cast to this type in order to
 * execute it. */
typedef void (* memory_write_function)(uint32_t, uint32_t);

/*==================[external function declarations]==========================*/
/*==================[internal function declarations]==========================*/
/*==================[external constants]======================================*/
/*==================[internal constants]======================================*/
/*==================[external data]===========================================*/
/*==================[internal data]===========================================*/

/*
 * From "MPC5748G Reference Manual", 73.4 Flash memory controller memory map:
 *
 * <<Attempted updates to the programming model while the flash memory controller
 * module is in the midst of an operation will result in non-deterministic
 * behavior. Software must be architected to avoid this scenario. There is no
 * idle indicator for the flash controller. The recommended flow for multi-core
 * devices is to start only one core and execute initialization code to
 * completion before starting the remaining cores. If the user needs to
 * reconfigure the flash, code execution must be temporarily moved to system
 * RAM.>>
 *
 * This means the code used to write the registers of the flash controller must
 * be executed from SRAM (because the flash containing the code is being
 * reconfigured which might produce unexpected behavior). The array
 * flash_write_machine_code stores this code. It has been created by assembling
 * the following code and copypasting the output of objdump:
 *
 *    e_stw r3, 0(r4) // write content of r3 at address in r4
 *    mbar            // wait for completion
 *    se_blr          // return
 *    se_nop          // nothing, just padding
 *
 * The function takes 2 arguments:
 *  - value to write in r3
 *  - address where to write the value in r4
 *
 * r3 and r4 are specified as first and second arguments by the powerpc ABI.
 * The last nop instruction is just to pad the instructions to a multiple of 4
 * bytes.
 */

unsigned char flash_write_machine_code[] __aligned(32) =
{
    0x54, 0x64, 0x00, 0x00,
    0x7c, 0x00, 0x06, 0xac,
    0x00, 0x04,
    0x44, 0x00
};

/*==================[external function definitions]===========================*/


void board_config_memory(void)
{
    memory_write_function flash_write = (memory_write_function) flash_write_machine_code;

    flash_write(PFLASH_PFCR1_VALUE_160MHz, PFLASH_PFCR1);
    flash_write(PFLASH_PFCR2_VALUE_BASIC,  PFLASH_PFCR2);
    flash_write(PFLASH_PFCR3_VALUE_BASIC,  PFLASH_PFCR3);
    flash_write(PFLASH_PFAPR_VALUE_BASIC,  PFLASH_PFAPR);
    flash_write(PFLASH_PFCR4_VALUE_BASIC,  PFLASH_PFCR4);

    /* RAM read data is passed directly to the system bus,
     * incurring no additional latency */
    WRITE32(0, PRAMC_0_PRCR1);
    WRITE32(0, PRAMC_1_PRCR1);
    WRITE32(0, PRAMC_2_PRCR1);
}

/*==================[internal function definitions]===========================*/
/*==================[end of file]=============================================*/
